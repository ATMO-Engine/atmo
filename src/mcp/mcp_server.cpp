#include "mcp_server.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <random>

#include "core/ecs/entities/entity.hpp"
#include "core/engine.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/progress_tick_event/progress_tick_event.hpp"
#include "mcp/thread_pool_task_queue.hpp"
#include "mcp/tools/tools.hpp"
#include "project/project_settings.hpp"
#include "spdlog/spdlog.h"

namespace atmo::mcp
{
    namespace
    {
        constexpr std::array supported_protocol_versions{ std::string_view("2025-11-25"), std::string_view("2025-06-18"), std::string_view("2025-03-26") };

        constexpr int parse_error = -32700;
        constexpr int invalid_request = -32600;
        constexpr int method_not_found = -32601;
        constexpr int invalid_params = -32602;

        glz::generic ErrorResponse(const glz::generic &id, int code, std::string message)
        {
            return glz::generic::object_t{
                { "jsonrpc", "2.0" },
                { "id", id },
                { "error", glz::generic::object_t{ { "code", static_cast<double>(code) }, { "message", std::move(message) } } },
            };
        }

        glz::generic ResultResponse(const glz::generic &id, glz::generic result)
        {
            return glz::generic::object_t{ { "jsonrpc", "2.0" }, { "id", id }, { "result", std::move(result) } };
        }

        std::string NewSessionId()
        {
            static std::mutex mutex;
            static std::mt19937_64 generator{ std::random_device{}() };
            std::lock_guard lock(mutex);
            return std::format("{:016x}{:016x}", generator(), generator());
        }

        bool IsLocalOrigin(std::string_view origin)
        {
            for (std::string_view prefix :
                 { "http://127.0.0.1", "http://localhost", "http://[::1]", "https://127.0.0.1", "https://localhost", "https://[::1]" }) {
                if (!origin.starts_with(prefix))
                    continue;
                std::string_view rest = origin.substr(prefix.size());
                if (rest.empty() || rest.front() == ':' || rest.front() == '/')
                    return true;
            }
            return false;
        }

        std::string Instructions(McpServer::Mode mode)
        {
            if (mode == McpServer::Mode::Game)
                return "Atmo running game. Scene tools (scene_tree, entity_get, component_set...) act on the running scene, so omit their 'scene' "
                       "argument. Use game_pause / game_step to advance time deterministically, game_input and game_action to press keys, "
                       "luau_eval to inspect script state and logs_read to see what the engine printed.";
            return "Atmo editor. Paths are relative to the project root (or project:// / absolute). Scene tools take the scene file path in 'scene' "
                   "and open a scene editor tab for it when none is open; editor_close_auto closes the clean tabs opened that way. Entities are "
                   "addressed by their path below the scene root ('Player/Sprite') or by the numeric id returned by other tools. Save with "
                   "scene_save or editor_save. play_start launches the game in a child process with its own MCP server, reachable through "
                   "game_call. Screenshots need a GPU rendering platform (start with --rendering-platform=metal or vulkan in headless mode).";
        }
    } // namespace

    McpServer::McpServer(core::Engine &engine, Mode mode) : m_engine(engine), m_mode(mode)
    {
        RegisterTools(*this);
    }

    McpServer::~McpServer()
    {
        stop();
    }

    bool McpServer::start(const std::string &host, int port)
    {
        if (m_http)
            return true;

        auto http = std::make_unique<httplib::Server>();
        http->new_task_queue = [] { return new ThreadPoolTaskQueue(); };
        http->set_keep_alive_max_count(1);
        http->set_payload_max_length(16 * 1024 * 1024);
        http->Post("/mcp", [this](const httplib::Request &request, httplib::Response &response) { handleHttp(request, response); });
        http->Get("/mcp", [](const httplib::Request &, httplib::Response &response) {
            response.status = 405;
            response.set_header("Allow", "POST, DELETE");
        });
        http->Delete("/mcp", [this](const httplib::Request &request, httplib::Response &response) {
            std::lock_guard lock(m_session_mutex);
            m_sessions.erase(request.get_header_value("Mcp-Session-Id"));
            response.status = 200;
        });

        if (port == 0) {
            m_port = http->bind_to_any_port(host);
            if (m_port <= 0) {
                spdlog::error("MCP: could not listen on {}", host);
                return false;
            }
        } else {
            if (!http->bind_to_port(host, port)) {
                spdlog::error("MCP: could not listen on {}:{} (port in use?)", host, port);
                return false;
            }
            m_port = port;
        }

        if (core::ThreadPool::Instance().getWorkerCount() < 3)
            spdlog::warn("MCP: the thread pool has {} workers; the MCP server keeps one busy", core::ThreadPool::Instance().getWorkerCount());

        m_http = std::move(http);
        m_listen_task = core::ThreadPool::Instance().submit([server = m_http.get()] { server->listen_after_bind(); });

        m_tick_callback =
            core::event::EventRegistry::SetCallBack<core::event::events::ProgressTickEvent>([this](core::event::events::ProgressTickEvent *) { tick(); });
        m_has_tick_callback = true;

        spdlog::info("MCP server ({}) listening on http://{}:{}/mcp", m_mode == Mode::Editor ? "editor" : "game", host, m_port);
        return true;
    }

    void McpServer::stop()
    {
        m_alive->store(false);
        failInflight("The engine is shutting down");

        if (m_http) {
            m_http->stop();
            if (m_listen_task)
                m_listen_task->wait();
            m_listen_task.reset();
            m_http.reset();
        }

        if (m_has_tick_callback) {
            core::event::EventRegistry::RemoveCallBack<core::event::events::ProgressTickEvent>(m_tick_callback);
            m_has_tick_callback = false;
        }
        m_scheduled.clear();
    }

    void McpServer::failInflight(const std::string &message)
    {
        std::lock_guard lock(m_inflight_mutex);
        for (const auto &reply : m_inflight) reply.error(message);
    }

    void McpServer::after(std::uint64_t frames, std::function<void()> fn)
    {
        m_scheduled.emplace_back(m_ticks + frames + (m_in_tick ? 0 : 1), std::move(fn));
    }

    void McpServer::tick()
    {
        ++m_ticks;
        std::vector<std::function<void()>> due;
        for (auto it = m_scheduled.begin(); it != m_scheduled.end();) {
            if (it->first <= m_ticks) {
                due.push_back(std::move(it->second));
                it = m_scheduled.erase(it);
            } else {
                ++it;
            }
        }
        m_in_tick = true;
        for (auto &fn : due) fn();
        m_in_tick = false;
    }

    void McpServer::handleHttp(const httplib::Request &request, httplib::Response &response)
    {
        if (request.has_header("Origin") && !IsLocalOrigin(request.get_header_value("Origin"))) {
            response.status = 403;
            response.set_content("Forbidden origin", "text/plain");
            return;
        }

        const std::string session = request.get_header_value("Mcp-Session-Id");
        if (!session.empty()) {
            std::lock_guard lock(m_session_mutex);
            if (!m_sessions.contains(session)) {
                response.status = 404;
                response.set_content("Unknown session", "text/plain");
                return;
            }
        }

        bool initialize = false;
        std::optional<std::string> body = handleMessage(request.body, &initialize);
        if (initialize) {
            std::string id = NewSessionId();
            {
                std::lock_guard lock(m_session_mutex);
                m_sessions.insert(id);
            }
            response.set_header("Mcp-Session-Id", id);
        }

        if (!body) {
            response.status = 202;
            return;
        }
        response.status = 200;
        response.set_content(*body, "application/json");
    }

    std::optional<std::string> McpServer::handleMessage(std::string_view body, bool *initialize)
    {
        if (initialize)
            *initialize = false;

        glz::generic request;
        if (auto ec = glz::read_json(request, body))
            return ErrorResponse(nullptr, parse_error, "Parse error: " + glz::format_error(ec, body)).dump().value_or("{}");

        glz::generic response = dispatch(request, initialize);
        if (response.is_null())
            return std::nullopt;
        return response.dump().value_or("{}");
    }

    glz::generic McpServer::dispatch(const glz::generic &request, bool *initialize)
    {
        if (!request.is_object() || !request.contains("method") || !request["method"].is_string())
            return ErrorResponse(request.is_object() && request.contains("id") ? request["id"] : glz::generic{}, invalid_request, "Invalid request");

        const std::string &method = request["method"].get_string();
        const bool is_notification = !request.contains("id");
        const glz::generic id = is_notification ? glz::generic{} : request["id"];
        const glz::generic params = request.contains("params") ? request["params"] : glz::generic::object_t{};

        if (is_notification)
            return {};

        if (method == "initialize") {
            if (initialize)
                *initialize = true;
            std::string version(latest_protocol_version);
            if (params.is_object() && params.contains("protocolVersion") && params["protocolVersion"].is_string()) {
                const std::string &requested = params["protocolVersion"].get_string();
                if (std::find(supported_protocol_versions.begin(), supported_protocol_versions.end(), requested) != supported_protocol_versions.end())
                    version = requested;
            }
            return ResultResponse(
                id,
                glz::generic::object_t{
                    { "protocolVersion", version },
                    { "capabilities", glz::generic::object_t{ { "tools", glz::generic::object_t{ { "listChanged", false } } } } },
                    { "serverInfo", glz::generic::object_t{ { "name", "atmo" }, { "version", ATMO_VERSION } } },
                    { "instructions", Instructions(m_mode) },
                });
        }

        if (method == "ping")
            return ResultResponse(id, glz::generic::object_t{});

        if (method == "tools/list") {
            glz::generic::array_t tools;
            for (const auto &tool : m_tools.all())
                tools.push_back(glz::generic::object_t{ { "name", tool.name }, { "description", tool.description }, { "inputSchema", tool.input_schema } });
            return ResultResponse(id, glz::generic::object_t{ { "tools", std::move(tools) } });
        }

        if (method == "tools/call")
            return callTool(params, id);

        return ErrorResponse(id, method_not_found, "Method not found: " + method);
    }

    glz::generic McpServer::callTool(const glz::generic &params, const glz::generic &id)
    {
        if (!params.is_object() || !params.contains("name") || !params["name"].is_string())
            return ErrorResponse(id, invalid_params, "tools/call needs a tool name");

        const std::string &name = params["name"].get_string();
        const Tool *tool = m_tools.find(name);
        if (!tool)
            return ErrorResponse(id, invalid_params, "Unknown tool: " + name);

        glz::generic arguments = params.contains("arguments") ? params["arguments"] : glz::generic::object_t{};
        if (arguments.is_null())
            arguments = glz::generic::object_t{};

        Reply reply;
        std::list<Reply>::iterator inflight;
        {
            std::lock_guard lock(m_inflight_mutex);
            inflight = m_inflight.insert(m_inflight.end(), reply);
        }

        if (!m_alive->load()) {
            reply.error("The engine is shutting down");
        } else if (tool->main_thread) {
            core::SignalQueue::Enqueue([alive = m_alive, handler = tool->handler, arguments, reply] {
                if (alive->load())
                    handler(arguments, reply);
            });
        } else {
            tool->handler(arguments, reply);
        }

        std::optional<ToolResult> result = reply.wait(m_timeout);
        {
            std::lock_guard lock(m_inflight_mutex);
            m_inflight.erase(inflight);
        }
        if (!result)
            result = ToolResult::Error(std::format("Tool '{}' did not complete within {} ms", name, m_timeout.count()));

        return ResultResponse(id, result->toJson());
    }
} // namespace atmo::mcp
