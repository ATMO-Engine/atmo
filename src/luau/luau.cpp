#include "luau.hpp"
#include "bindings/bindings_input.hpp"
#include "ctre.hpp"
#include "lua.h"
#include "luacode.h"
#include "lualib.h"
#include "luau/bindings/EntityLuauRegistry.hpp"
#include "script_instance.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {
        Luau::Luau()
        {
            p_L = luaL_newstate();
            luaopen_base(p_L);
            luaopen_coroutine(p_L);
            luaopen_table(p_L);
            luaopen_string(p_L);
            luaopen_math(p_L);
            luaopen_utf8(p_L);
            luaopen_bit32(p_L);
            luaopen_buffer(p_L);
            luaopen_vector(p_L);

            registerBindings();

            luaL_sandbox(p_L);
        }

        Luau::~Luau()
        {
            if (p_L)
                lua_close(p_L);
            p_L = nullptr;
        }


        void Luau::LogCompileTimeError(const std::string &errorMsg, const std::string &context)
        {
            if (auto m = ctre::match<"^:(\\d+): (.*)$">(errorMsg)) {
                spdlog::error("[Luau] Compile error ({}) \n\t line: {} \n\t message: {}", context, m.get<1>().to_string(), m.get<2>().to_string());
            } else {
                spdlog::error("[Luau] Compile error ({}): {}", context, errorMsg);
            }
        }

        namespace
        {
            thread_local std::string s_lastTraceback;

            int MessageHandler(lua_State *L)
            {
                const char *trace = lua_debugtrace(L);
                s_lastTraceback = trace ? trace : "";

                const size_t firstLineEnd = s_lastTraceback.find('\n');
                s_lastTraceback.erase(0, firstLineEnd == std::string::npos ? std::string::npos : firstLineEnd + 1);
                return 1;
            }
        } // namespace

        bool Luau::ProtectedCall(lua_State *L, int nargs, const std::string &context)
        {
            const int handlerIndex = lua_gettop(L) - nargs;
            lua_pushcfunction(L, MessageHandler, "messageHandler");
            lua_insert(L, handlerIndex);

            s_lastTraceback.clear();
            const int status = lua_pcall(L, nargs, 0, handlerIndex);
            if (status != LUA_OK) {
                LogLuauError(L, context, s_lastTraceback);
            }

            lua_remove(L, handlerIndex);
            return status == LUA_OK;
        }

        void Luau::LogLuauError(lua_State *L, const std::string &context, const std::string &traceback)
        {
            const char *rawError = lua_tostring(L, -1);
            std::string errorMsg = rawError ? rawError : "unknown error (no message on stack)";
            lua_pop(L, 1);

            std::string trace;
            if (!traceback.empty()) {
                trace = "\n\t traceback:";
                std::string_view rest = traceback;
                while (!rest.empty()) {
                    const size_t end = rest.find('\n');
                    const std::string_view line = rest.substr(0, end);
                    if (!line.empty()) {
                        trace += "\n\t\t";
                        trace += line;
                    }
                    rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end + 1);
                }
            }

            if (auto m = ctre::match<"^(.*):(\\d+): (.*)$">(errorMsg)) {
                spdlog::error(
                    "[Luau] Error ({}) \n\t file: '{}' \n\t line: {} \n\t message: {}{}",
                    context,
                    m.get<1>().to_string(),
                    m.get<2>().to_string(),
                    m.get<3>().to_string(),
                    trace);
            } else {
                spdlog::error("[Luau] Error ({}): {}{}", context, errorMsg, trace);
            }
        }

        char *Luau::Compile(const std::string &source, size_t *bytecode_size, const std::string &chunkName, lua_CompileOptions *options)
        {
            char *bytecode = luau_compile(source.c_str(), source.size(), options, bytecode_size);

            if (bytecode != nullptr && *bytecode_size > 0 && bytecode[0] == 0) {
                std::string errorMsg(bytecode + 1, *bytecode_size - 1);
                free(bytecode);

                LogCompileTimeError(errorMsg, chunkName);

                *bytecode_size = 0;
                return nullptr;
            }

            return bytecode;
        }

        bool Luau::loadBytecode(const std::string &name, const char *code, size_t size, int env)
        {
            const int result = luau_load(p_L, name.c_str(), code, size, env);
            if (result != 0) {
                LogLuauError(p_L, name);
                return false;
            }
            return true;
        }

        bool Luau::LoadBytecodeCoroutine(lua_State *coroutine, const std::string &name, const char *code, size_t size, int env)
        {
            const int result = luau_load(coroutine, name.c_str(), code, size, env);
            if (result != 0) {
                LogLuauError(coroutine, name);
                return false;
            }
            return true;
        }

        ScriptInstance *Luau::generateInstance()
        {
            return new ScriptInstance(this);
        }

        void Luau::registerBindings()
        {
            spdlog::debug("Bindings registration start:");

            lua_pushcfunction(
                p_L,
                [](lua_State *L) -> int {
                    InputBindings::RegisterType(L);
                    LuauRegistry::Instance().registerAll(L);
                    return 0;
                },
                "registerBindings");
            if (!ProtectedCall(p_L, 0, "bindings registration")) {
                return;
            }

            spdlog::debug("Bindings registration finished");
        }

        void Luau::registerModule(const std::string &name, lua_CFunction loader)
        {
            loader(p_L);
            lua_setglobal(p_L, name.c_str());
        }
    } // namespace luau
} // namespace atmo
