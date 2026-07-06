#include "luau.hpp"
#include "bindings/bindings_color.hpp"
#include "bindings/bindings_entity.hpp"
#include "bindings/bindings_input.hpp"
#include "bindings/bindings_transform2.hpp"
#include "bindings/bindings_vector2.hpp"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/types.hpp"
#include "ctre.hpp"
#include "lua.h"
#include "luacode.h"
#include "lualib.h"
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

        void Luau::LogLuauError(lua_State *L, const std::string &context)
        {
            const char *rawError = lua_tostring(L, -1);
            std::string errorMsg = rawError ? rawError : "unknown error (no message on stack)";
            lua_pop(L, 1);

            if (auto m = ctre::match<"^(.*):(\\d+): (.*)$">(errorMsg)) {
                spdlog::error(
                    "[Luau] Error ({}) \n\t file: '{}' \n\t line: {} \n\t message: {}",
                    context,
                    m.get<1>().to_string(),
                    m.get<2>().to_string(),
                    m.get<3>().to_string());
            } else {
                spdlog::error("[Luau] Error ({}): {}", context, errorMsg);
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
            LuaBindings<atmo::core::types::Vector2>::RegisterType(p_L);
            LuaBindings<atmo::core::types::Color>::RegisterType(p_L);
            LuaBindings<atmo::core::components::Transform2d>::RegisterType(p_L);
            LuaBindings<flecs::entity>::RegisterType(p_L);

            InputBindings::RegisterType(p_L);
            return;
        }

        void Luau::registerModule(const std::string &name, lua_CFunction loader)
        {
            loader(p_L);
            lua_setglobal(p_L, name.c_str());
        }
    } // namespace luau
} // namespace atmo
