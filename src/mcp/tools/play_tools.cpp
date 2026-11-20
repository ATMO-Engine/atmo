#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "core/engine.hpp"
#include "core/thread/thread_pool.hpp"
#include "editor/editor_manager.hpp"
#include "httplib.h"
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

        std::expected<glz::generic, std::string> Request(int port, const std::string &method, glz::generic params, std::chrono::seconds timeout)
        {
            if (port <= 0)
                return std::unexpected("The game is not running; start it with play_start");
            httplib::Client client("127.0.0.1", port);
            client.set_connection_timeout(std::chrono::seconds(2));
            client.set_read_timeout(timeout);
            const glz::generic body = glz::generic::object_t{ { "jsonrpc", "2.0" }, { "id", 1.0 }, { "method", method }, { "params", std::move(params) } };
            auto response = client.Post("/mcp", body.dump().value_or("{}"), "application/json");
            if (!response)
                return std::unexpected("The game's MCP server did not answer on port " + std::to_string(port) + "; is the game still running (play_status)?");
            auto json = glz::read_json<glz::generic>(response->body);
            if (!json || !json->is_object())
                return std::unexpected("Invalid answer from the game: " + response->body);
            if (json->contains("error"))
                return std::unexpected("The game answered: " + (*json)["error"].dump().value_or(""));
            if (!json->contains("result"))
                return std::unexpected("Invalid answer from the game: " + response->body);
            return (*json)["result"];
        }
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

                core::ThreadPool::Instance().submit([reply, port] {
                    const auto deadline = std::chrono::steady_clock::now() + ready_timeout;
                    while (std::chrono::steady_clock::now() < deadline && !reply.sent()) {
                        if (Request(port, "ping", glz::generic::object_t{}, std::chrono::seconds(2))) {
                            reply.json(glz::generic::object_t{ { "running", true }, { "port", static_cast<double>(port) }, { "ready", true } });
                            return;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    reply.json(
                        glz::generic::object_t{
                            { "running", true },
                            { "port", static_cast<double>(port) },
                            { "ready", false },
                            { "hint", "The game's MCP server did not answer yet; check play_status and logs_read" },
                        });
                });
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
            [state](const NoArgs &, const Reply &reply) {
                auto result = Request(state->port.load(), "tools/list", glz::generic::object_t{}, std::chrono::seconds(10));
                if (!result)
                    return reply.error(result.error());
                glz::generic::array_t tools;
                if (result->contains("tools") && (*result)["tools"].is_array())
                    for (const auto &tool : (*result)["tools"].get_array())
                        tools.push_back(
                            glz::generic::object_t{ { "name", tool["name"] }, { "description", tool["description"] }, { "inputSchema", tool["inputSchema"] } });
                reply.json(glz::generic::object_t{ { "tools", std::move(tools) } });
            },
            false);

        tools.add<GameCallArgs>(
            "game_call",
            "Call a tool of the game started with play_start and return its result.",
            [state](const GameCallArgs &args, const Reply &reply) {
                auto result = Request(
                    state->port.load(),
                    "tools/call",
                    glz::generic::object_t{ { "name", args.tool }, { "arguments", args.arguments.value_or(glz::generic::object_t{}) } },
                    std::chrono::seconds(35));
                if (!result)
                    return reply.error(result.error());
                ToolResult forwarded;
                if (result->contains("content") && (*result)["content"].is_array())
                    forwarded.content = (*result)["content"].get_array();
                forwarded.is_error = result->contains("isError") && (*result)["isError"].is_boolean() && (*result)["isError"].get_boolean();
                reply.send(std::move(forwarded));
            },
            false);
    }
} // namespace atmo::mcp::tools
