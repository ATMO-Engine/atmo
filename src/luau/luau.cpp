#include "Exposed/time.hpp"
#include "Exposed/vector2.hpp"

#include "script_instance.hpp"
#include "spdlog/spdlog.h"

#include "lua.h"
#include "luacode.h"
#include "lualib.h"
#include "luau.hpp"

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

        char *Luau::Compile(const std::string &source, size_t *bytecode_size, lua_CompileOptions *options)
        {
            return luau_compile(source.c_str(), source.size(), options, bytecode_size);
        }

        bool Luau::loadBytecode(const std::string &name, const char *code, size_t size, int env)
        {
            const int result = luau_load(p_L, name.c_str(), code, size, env);
            if (result != 0) {
                spdlog::error("Failed to load Lua code:" + name);
                return false;
            }
            return true;
        }

        bool Luau::LoadBytecodeCoroutine(lua_State *coroutine, const std::string &name, const char *code, size_t size, int env)
        {
            const int result = luau_load(coroutine, name.c_str(), code, size, env);
            if (result != 0) {
                spdlog::error("Failed to load Lua code:" + name);
                return false;
            }
            return true;
        }

        ScriptInstance Luau::generateInstance()
        {
            return ScriptInstance(*this);
        }

        void Luau::registerBindings()
        {
            // static method to register exposed class
            Vector2::RegisterVector2(p_L);
            Time::RegisterTime(p_L);
            return;
        }

        void Luau::registerModule(const std::string &name, lua_CFunction loader)
        {
            loader(p_L);
            lua_setglobal(p_L, name.c_str());
        }
    } // namespace luau
} // namespace atmo
