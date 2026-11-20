#include <atomic>
#include <chrono>
#include <expected>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/engine.hpp"
#include "core/thread/thread_pool.hpp"
#include "editor/editor_manager.hpp"
#include "mcp/http/http_client.hpp"
#include "mcp/mcp_server.hpp"
#include "mcp/tools/helpers.hpp"
#include "mcp/tools/tools.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::mcp::tools
{
    namespace play_args
    {
        struct PlayStartArgs {
            std::optional<std::string> scene;
            std::optional<bool> headless;
            std::optional<int> port;
        };

        struct GameCallArgs {
            std::string tool;
            std::optional<glz::generic> arguments;
        };

        struct PlayState {
            std::atomic<int> port{ 0 };
        };

        constexpr auto ready_timeout = std::chrono::seconds(20);
        constexpr auto ready_retry = std::chrono::milliseconds(100);

        using Answer = std::expected<glz::generic, std::string>;

        void Request(
            http::HttpClient &client, int port, const std::string &method, glz::generic params, std::chrono::milliseconds timeout,
            std::function<void(Answer)> done)
        {
            if (port <= 0)
                return done(std::unexpected("The game is not running; start it with play_start"));

            const glz::generic body = glz::generic::object_t{ { "jsonrpc", "2.0" }, { "id", 1.0 }, { "method", method }, { "params", std::move(params) } };
            http::ClientRequest request;
            request.method = "POST";
            request.url = std::format("http://127.0.0.1:{}/mcp", port);
            request.headers = { { "Content-Type", "application/json" }, { "Accept", "application/json, text/event-stream" } };
            request.body = body.dump().value_or("{}");
            request.connect_timeout = std::chrono::seconds(2);
            request.timeout = timeout;
            request.use_proxy = false;

            client.request(std::move(request), [port, done = std::move(done)](http::ClientResponse response) {
                if (!response.ok())
                    return done(
                        std::unexpected(
                            std::format(
                                "The game's MCP server did not answer on port {} ({}); is the game still running (play_status)?", port, response.error)));
                if (response.status != 200)
                    return done(std::unexpected(std::format("The game answered HTTP {}: {}", response.status, response.body)));
                auto json = glz::read_json<glz::generic>(response.body);
                if (!json || !json->is_object())
                    return done(std::unexpected("Invalid answer from the game: " + response.body));
                if (json->contains("error"))
                    return done(std::unexpected("The game answered: " + (*json)["error"].dump().value_or("")));
                if (!json->contains("result"))
                    return done(std::unexpected("Invalid answer from the game: " + response.body));
                done((*json)["result"]);
            });
        }

        struct ReadyProbe : std::enable_shared_from_this<ReadyProbe> {
            std::shared_ptr<http::HttpClient> client;
            Reply reply;
            int port = 0;
            std::chrono::steady_clock::time_point deadline;

            void attempt()
            {
                if (reply.sent())
                    return;
                Request(*client, port, "ping", glz::generic::object_t{}, std::chrono::seconds(2), [self = shared_from_this()](const Answer &answer) {
                    if (self->reply.sent())
                        return;
                    if (answer)
                        return self->reply.json(glz::generic::object_t{ { "running", true }, { "port", static_cast<double>(self->port) }, { "ready", true } });
                    if (std::chrono::steady_clock::now() >= self->deadline)
                        return self->reply.json(
                            glz::generic::object_t{
                                { "running", true },
                                { "port", static_cast<double>(self->port) },
                                { "ready", false },
                                { "hint", "The game's MCP server did not answer yet; check play_status and logs_read" },
                            });
                    core::ThreadPool::Instance().submitAfter([self] { self->attempt(); }, ready_retry);
                });
            }
        };
    } // namespace play_args
} // namespace atmo::mcp::tools

using namespace atmo::mcp::tools::play_args;

template <> struct glz::json_schema<PlayStartArgs> {
    schema scene{ .description = "Scene to run (default: the project's default scene)" };
    schema headless{ .description = "Run the game without a window (default false)" };
    schema port{ .description = "Port of the game's MCP server (default: this server's port + 1)" };
};
template <> struct glz::json_schema<GameCallArgs> {
    schema tool{ .description = "Tool of the running game (see game_tools): game_status, game_step, game_input, scene_tree, entity_get, luau_eval..." };
    schema arguments{ .description = "Arguments of that tool" };
};

namespace atmo::mcp::tools
{
    void RegisterPlayTools(McpServer &server)
    {
        auto &tools = server.tools();
        auto state = std::make_shared<PlayState>();

        tools.add<PlayStartArgs>(
            "play_start",
            "Save the modified scenes and run the game in a child process with its own MCP server; game_call then drives it.",
            [&server, state](const PlayStartArgs &args, const Reply &reply) {
                auto *manager = server.engine().editor();
                if (manager->isPlaying())
                    return reply.error("The game is already running (play_stop first)");

                for (const auto &editor : manager->editors()) {
                    if (editor->isDirty() && editor->hasFilePath()) {
                        editor->save();
                        editor->clearDirty();
                    }
                }

                const int port = args.port.value_or(server.port() + 1);
                std::vector<std::string> extra{ "--mcp", std::to_string(port) };
                if (args.headless.value_or(false)) {
                    extra.emplace_back("--headless");
                    const auto platform = server::rendering::RenderingServer::Instance().platformKind();
                    if (platform != server::rendering::PlatformKind::Null) {
                        extra.emplace_back("--rendering-platform");
                        extra.emplace_back(server::rendering::PlatformKindName(platform));
                    }
                }

                std::optional<std::string> scene;
                if (args.scene)
                    scene = ResolvePath(*args.scene).string();
                if (!manager->startPlay(scene, extra))
                    return reply.error("Could not start the game (see logs_read)");
                state->port = port;

                auto probe = std::make_shared<ReadyProbe>();
                probe->client = server.httpClient();
                probe->reply = reply;
                probe->port = port;
                probe->deadline = std::chrono::steady_clock::now() + ready_timeout;
                probe->attempt();
            });

        tools.add<NoArgs>("play_stop", "Stop the game started with play_start.", [&server, state](const NoArgs &, const Reply &reply) {
            server.engine().editor()->stopPlay();
            state->port = 0;
            reply.json(glz::generic::object_t{ { "running", false } });
        });

        tools.add<NoArgs>("play_status", "Tell whether the game started with play_start is running.", [&server, state](const NoArgs &, const Reply &reply) {
            const bool running = server.engine().editor()->isPlaying();
            if (!running)
                state->port = 0;
            reply.json(glz::generic::object_t{ { "running", running }, { "port", static_cast<double>(state->port.load()) } });
        });

        tools.add<NoArgs>(
            "game_tools",
            "List the tools of the running game, for game_call.",
            [&server, state](const NoArgs &, const Reply &reply) {
                Request(
                    *server.httpClient(), state->port.load(), "tools/list", glz::generic::object_t{}, std::chrono::seconds(10), [reply](const Answer &result) {
                        if (!result)
                            return reply.error(result.error());
                        glz::generic::array_t tools;
                        if (result->contains("tools") && (*result)["tools"].is_array())
                            for (const auto &tool : (*result)["tools"].get_array())
                                tools.push_back(
                                    glz::generic::object_t{
                                        { "name", tool["name"] }, { "description", tool["description"] }, { "inputSchema", tool["inputSchema"] } });
                        reply.json(glz::generic::object_t{ { "tools", std::move(tools) } });
                    });
            },
            false);

        tools.add<GameCallArgs>(
            "game_call",
            "Call a tool of the game started with play_start and return its result.",
            [&server, state](const GameCallArgs &args, const Reply &reply) {
                Request(
                    *server.httpClient(),
                    state->port.load(),
                    "tools/call",
                    glz::generic::object_t{ { "name", args.tool }, { "arguments", args.arguments.value_or(glz::generic::object_t{}) } },
                    std::chrono::seconds(35),
                    [reply](const Answer &result) {
                        if (!result)
                            return reply.error(result.error());
                        ToolResult forwarded;
                        if (result->contains("content") && (*result)["content"].is_array())
                            forwarded.content = (*result)["content"].get_array();
                        forwarded.is_error = result->contains("isError") && (*result)["isError"].is_boolean() && (*result)["isError"].get_boolean();
                        reply.send(std::move(forwarded));
                    });
            },
            false);
    }
} // namespace atmo::mcp::tools
