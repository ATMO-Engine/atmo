#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>

#include "spdlog/spdlog.h"

#include "lua.h"
#include "luacode.h"
#include "lualib.h"
#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau/luau.hpp"
#include "mcp/log_buffer.hpp"
#include "mcp/mcp_server.hpp"
#include "mcp/tools/helpers.hpp"
#include "mcp/tools/tools.hpp"

namespace atmo::mcp::tools
{
    namespace common_args
    {
        struct LogsReadArgs {
            std::optional<std::uint64_t> since;
            std::optional<std::string> level;
            std::optional<int> limit;
        };

        struct LuauEvalArgs {
            std::string code;
            std::optional<std::string> scene;
            std::optional<std::string> entity;
        };

        glz::generic::array_t LogsJson(std::uint64_t since, spdlog::level::level_enum level, std::size_t limit)
        {
            glz::generic::array_t entries;
            for (auto &entry : LogBuffer::Read(since, level, limit))
                entries.push_back(
                    glz::generic::object_t{
                        { "seq", static_cast<double>(entry.seq) },
                        { "time", std::move(entry.time) },
                        { "level", std::move(entry.level) },
                        { "message", std::move(entry.message) },
                    });
            return entries;
        }

        struct EvalResult {
            std::optional<std::string> error;
            glz::generic::array_t values;
            glz::generic::array_t printed;
        };

        thread_local glz::generic::array_t *s_printed = nullptr;

        int CapturePrint(lua_State *state)
        {
            std::string line;
            const int count = lua_gettop(state);
            for (int i = 1; i <= count; ++i) {
                std::size_t length = 0;
                const char *text = luaL_tolstring(state, i, &length);
                if (i > 1)
                    line += '\t';
                line.append(text, length);
                lua_pop(state, 1);
            }
            if (s_printed)
                s_printed->emplace_back(std::move(line));
            return 0;
        }

        EvalResult Evaluate(const std::string &code, std::optional<flecs::entity> self)
        {
            EvalResult result;
            std::size_t size = 0;
            char *bytecode = luau_compile(code.c_str(), code.size(), nullptr, &size);
            if (!bytecode) {
                result.error = "Compilation failed";
                return result;
            }
            if (size > 0 && bytecode[0] == 0) {
                result.error = "Compile error" + std::string(bytecode + 1, size - 1);
                std::free(bytecode);
                return result;
            }

            lua_State *state = luau::Luau::Instance().getState();
            lua_State *thread = lua_newthread(state);
            const int thread_ref = lua_ref(state, -1);
            lua_pop(state, 1);
            luaL_sandboxthread(thread);
            lua_pushcfunction(thread, CapturePrint, "print");
            lua_setglobal(thread, "print");
            s_printed = &result.printed;

            if (self) {
                flecs::entity entity = *self;
                if (luau::LuauRegistry::Instance().pushEntity(thread, entity))
                    lua_setglobal(thread, "this");
            }

            const int loaded = luau_load(thread, "=mcp", bytecode, size, 0);
            std::free(bytecode);
            if (loaded != 0) {
                const char *message = lua_tostring(thread, -1);
                result.error = message ? message : "Could not load the chunk";
            } else {
                const int base = lua_gettop(thread) - 1;
                const int status = lua_pcall(thread, 0, LUA_MULTRET, 0);
                if (status != LUA_OK) {
                    const char *message = lua_tostring(thread, -1);
                    result.error = message ? message : "Runtime error";
                } else {
                    const int count = lua_gettop(thread) - base;
                    for (int i = 1; i <= count; ++i) {
                        std::size_t length = 0;
                        const char *text = luaL_tolstring(thread, base + i, &length);
                        result.values.emplace_back(std::string(text, length));
                        lua_pop(thread, 1);
                    }
                }
            }
            s_printed = nullptr;
            lua_settop(thread, 0);
            lua_unref(state, thread_ref);
            return result;
        }
    } // namespace common_args
} // namespace atmo::mcp::tools

using namespace atmo::mcp::tools::common_args;

template <> struct glz::json_schema<LogsReadArgs> {
    schema since{ .description = "Only return entries after this sequence number (the 'last' value of a previous call)" };
    schema level{ .description = "Minimum level: trace, debug, info (default), warning, error, critical" };
    schema limit{ .description = "Maximum number of entries, the most recent ones (default 200)" };
};
template <> struct glz::json_schema<LuauEvalArgs> {
    schema code{ .description = "Luau chunk; its return values are converted with tostring" };
    schema scene{ .description = "Scene used to resolve 'entity' (editor: scene file path)" };
    schema entity{ .description = "Entity bound to the global 'this', like in an entity script" };
};

namespace atmo::mcp::tools
{
    void RegisterCommonTools(McpServer &server)
    {
        auto &tools = server.tools();

        tools.add<LogsReadArgs>(
            "logs_read",
            "Read the engine log (the last 2000 entries are kept). Times are UTC.",
            [](const LogsReadArgs &args, const Reply &reply) {
                spdlog::level::level_enum level = spdlog::level::info;
                if (args.level) {
                    level = spdlog::level::from_str(*args.level == "warn" ? "warning" : *args.level);
                    if (level == spdlog::level::off && *args.level != "off")
                        return reply.error("Unknown level: " + *args.level);
                }
                const std::size_t limit = static_cast<std::size_t>(std::clamp(args.limit.value_or(200), 1, static_cast<int>(LogBuffer::capacity)));
                reply.json(
                    glz::generic::object_t{
                        { "last", static_cast<double>(LogBuffer::LastSequence()) },
                        { "entries", LogsJson(args.since.value_or(0), level, limit) },
                    });
            },
            false);

        tools.add<LuauEvalArgs>(
            "luau_eval",
            "Run Luau code inside the engine (arbitrary code: it can change or break the running state) and return its results, what it "
            "printed and the log lines it produced. Each call runs in a fresh sandboxed thread; scripts' globals are not shared.",
            [&server](const LuauEvalArgs &args, const Reply &reply) {
                std::optional<flecs::entity> self;
                if (args.entity) {
                    auto target = ResolveScene(server, args.scene);
                    if (!target)
                        return reply.error(target.error());
                    auto entity = ResolveEntity(*target, *args.entity);
                    if (!entity)
                        return reply.error(entity.error());
                    self = entity->getHandle();
                }

                const std::uint64_t before = LogBuffer::LastSequence();
                EvalResult result = Evaluate(args.code, self);
                glz::generic json = glz::generic::object_t{
                    { "results", std::move(result.values) },
                    { "printed", std::move(result.printed) },
                    { "logs", LogsJson(before, spdlog::level::trace, LogBuffer::capacity) },
                };
                if (result.error) {
                    json["error"] = *result.error;
                    ToolResult error = ToolResult::Json(json);
                    error.is_error = true;
                    return reply.send(std::move(error));
                }
                reply.json(json);
            });
    }
} // namespace atmo::mcp::tools
