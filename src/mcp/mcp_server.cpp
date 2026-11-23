#include "mcp_server.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <random>

#include "core/ecs/entities/entity.hpp"
#include "core/engine.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/progress_tick_event/progress_tick_event.hpp"
#include "mcp/http/http_client.hpp"
#include "mcp/http/http_server.hpp"
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

        constexpr std::chrono::milliseconds poll_interval{ 2 };

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

    McpServer::McpServer(core::Engine &engine, Mode mode) : m_engine(engine), m_mode(mode), m_client(std::make_shared<http::HttpClient>())
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

        auto server = std::make_unique<http::HttpServer>(
            [this](const http::HttpRequest &request, const http::HttpResponder &responder) { handleHttp(request, responder); });
        server->setMaxBodySize(16 * 1024 * 1024);
        if (!server->listen(host, port)) {
            if (port == 0)
                spdlog::error("MCP: could not listen on {}", host);
            else
                spdlog::error("MCP: could not listen on {}:{} (port in use?)", host, port);
            return false;
        }
        m_port = server->port();
        m_http = std::move(server);
        m_poll_task = core::ThreadPool::Instance().repeat([this] { poll(); }, -1, poll_interval);

        m_tick_callback =
            core::event::EventRegistry::SetCallBack<core::event::events::ProgressTickEvent>([this](core::event::events::ProgressTickEvent *) { tick(); });
        m_has_tick_callback = true;

        spdlog::info("MCP server ({}) listening on http://{}:{}/mcp", m_mode == Mode::Editor ? "editor" : "game", host, m_port);
        return true;
    }

    void McpServer::stop()
    {
        m_alive->store(false);

        if (m_poll_task) {
            m_poll_task->cancel();
            m_poll_task->wait();
            m_poll_task.reset();
        }

        failInflight("The engine is shutting down");
        m_client->cancelAll("The engine is shutting down");

        if (m_http) {
            m_http->close();
            m_http.reset();
        }

        if (m_has_tick_callback) {
            core::event::EventRegistry::RemoveCallBack<core::event::events::ProgressTickEvent>(m_tick_callback);
            m_has_tick_callback = false;
        }
        m_scheduled.clear();
    }

    void McpServer::poll()
    {
        m_http->step();
        m_client->step();
        expireInflight();
    }

    void McpServer::expireInflight()
    {
        const auto now = std::chrono::steady_clock::now();
        std::vector<Inflight> expired;
        {
            std::lock_guard lock(m_inflight_mutex);
            for (const auto &[key, inflight] : m_inflight)
                if (inflight.deadline <= now)
                    expired.push_back(inflight);
        }
        for (const auto &inflight : expired) inflight.reply.error(std::format("Tool '{}' did not complete within {} ms", inflight.tool, m_timeout.count()));
    }

    void McpServer::failInflight(const std::string &message)
    {
        std::unordered_map<std::uint64_t, Inflight> inflight;
        {
            std::lock_guard lock(m_inflight_mutex);
            inflight.swap(m_inflight);
        }
        for (const auto &[key, entry] : inflight) entry.reply.error(message);
    }

    void McpServer::after(std::uint64_t frames, std::function<void()> fn)
    {
        m_scheduled.emplace_back(m_ticks + frames + (m_in_tick ? 0 : 1), std::move(fn));
    }

    void McpServer::tick()
    {
        expireInflight();

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

    void McpServer::handleHttp(const http::HttpRequest &request, const http::HttpResponder &responder)
    {
        const auto text = [&responder](int status, std::string body) {
            responder.send(http::HttpResponse{ .status = status, .headers = { { "Content-Type", "text/plain" } }, .body = std::move(body) });
        };

        if (request.path != "/mcp")
            return text(404, "Not found");

        const std::string origin = request.header("Origin");
        if (!origin.empty() && !IsLocalOrigin(origin))
            return text(403, "Forbidden origin");

        if (request.method == "GET" || (request.method != "POST" && request.method != "DELETE")) {
            responder.send(http::HttpResponse{ .status = 405, .headers = { { "Allow", "POST, DELETE" } }, .body = {} });
            return;
        }

        const std::string session = request.header("Mcp-Session-Id");
        if (request.method == "DELETE") {
            std::lock_guard lock(m_session_mutex);
            m_sessions.erase(session);
            responder.send(http::HttpResponse{});
            return;
        }

        if (!session.empty()) {
            std::lock_guard lock(m_session_mutex);
            if (!m_sessions.contains(session))
                return text(404, "Unknown session");
        }

        auto initialize = std::make_shared<bool>(false);
        handleMessage(
            request.body,
            [this, responder, initialize](std::optional<std::string> body) {
                http::HttpResponse response;
                if (*initialize) {
                    std::string id = NewSessionId();
                    {
                        std::lock_guard lock(m_session_mutex);
                        m_sessions.insert(id);
                    }
                    response.headers.emplace_back("Mcp-Session-Id", std::move(id));
                }
                if (!body) {
                    response.status = 202;
                } else {
                    response.headers.emplace_back("Content-Type", "application/json");
                    response.body = std::move(*body);
                }
                responder.send(std::move(response));
            },
            initialize.get());
    }

    void McpServer::handleMessage(std::string_view body, MessageCallback done, bool *initialize)
    {
        if (initialize)
            *initialize = false;

        glz::generic request;
        if (auto ec = glz::read_json(request, body))
            return done(ErrorResponse(nullptr, parse_error, "Parse error: " + glz::format_error(ec, body)).dump().value_or("{}"));

        dispatch(request, initialize, [done = std::move(done)](glz::generic response) {
            if (response.is_null())
                return done(std::nullopt);
            done(response.dump().value_or("{}"));
        });
    }

    void McpServer::dispatch(const glz::generic &request, bool *initialize, const Respond &respond)
    {
        if (!request.is_object() || !request.contains("method") || !request["method"].is_string())
            return respond(ErrorResponse(request.is_object() && request.contains("id") ? request["id"] : glz::generic{}, invalid_request, "Invalid request"));

        const std::string &method = request["method"].get_string();
        const bool is_notification = !request.contains("id");
        const glz::generic id = is_notification ? glz::generic{} : request["id"];
        const glz::generic params = request.contains("params") ? request["params"] : glz::generic::object_t{};

        if (is_notification)
            return respond({});

        if (method == "initialize") {
            if (initialize)
                *initialize = true;
            std::string version(latest_protocol_version);
            if (params.is_object() && params.contains("protocolVersion") && params["protocolVersion"].is_string()) {
                const std::string &requested = params["protocolVersion"].get_string();
                if (std::find(supported_protocol_versions.begin(), supported_protocol_versions.end(), requested) != supported_protocol_versions.end())
                    version = requested;
            }
            return respond(ResultResponse(
                id,
                glz::generic::object_t{
                    { "protocolVersion", version },
                    { "capabilities", glz::generic::object_t{ { "tools", glz::generic::object_t{ { "listChanged", false } } } } },
                    { "serverInfo", glz::generic::object_t{ { "name", "atmo" }, { "version", ATMO_VERSION } } },
                    { "instructions", Instructions(m_mode) },
                }));
        }

        if (method == "ping")
            return respond(ResultResponse(id, glz::generic::object_t{}));

        if (method == "tools/list") {
            glz::generic::array_t tools;
            for (const auto &tool : m_tools.all())
                tools.push_back(glz::generic::object_t{ { "name", tool.name }, { "description", tool.description }, { "inputSchema", tool.input_schema } });
            return respond(ResultResponse(id, glz::generic::object_t{ { "tools", std::move(tools) } }));
        }

        if (method == "tools/call")
            return callTool(params, id, respond);

        respond(ErrorResponse(id, method_not_found, "Method not found: " + method));
    }

    void McpServer::callTool(const glz::generic &params, const glz::generic &id, const Respond &respond)
    {
        if (!params.is_object() || !params.contains("name") || !params["name"].is_string())
            return respond(ErrorResponse(id, invalid_params, "tools/call needs a tool name"));

        const std::string &name = params["name"].get_string();
        const Tool *tool = m_tools.find(name);
        if (!tool)
            return respond(ErrorResponse(id, invalid_params, "Unknown tool: " + name));

        glz::generic arguments = params.contains("arguments") ? params["arguments"] : glz::generic::object_t{};
        if (arguments.is_null())
            arguments = glz::generic::object_t{};

        Reply reply;
        std::uint64_t key = 0;
        {
            std::lock_guard lock(m_inflight_mutex);
            key = ++m_next_inflight;
            m_inflight.emplace(key, Inflight{ .reply = reply, .tool = name, .deadline = std::chrono::steady_clock::now() + m_timeout });
        }
        reply.onSent([this, key, id, respond](const ToolResult &result) {
            {
                std::lock_guard lock(m_inflight_mutex);
                m_inflight.erase(key);
            }
            respond(ResultResponse(id, result.toJson()));
        });

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
    }
} // namespace atmo::mcp
