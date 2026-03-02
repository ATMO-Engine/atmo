#pragma once

#include <cstddef>
#include <string>
#include "lua.h"
#include "luacode.h"

namespace atmo
{
    namespace luau
    {
        class ScriptInstance;


        class Luau
        {
        public:
            Luau();
            ~Luau();

            static char *Compile(const std::string &source, size_t *bytecode_size, lua_CompileOptions *options = nullptr);
            bool loadBytecode(const std::string &name, const char *code, size_t size, int env);
            static bool LoadBytecodeCoroutine(lua_State *coroutine, const std::string &name, const char *code, size_t size, int env);

            ScriptInstance generateInstance();

            constexpr inline lua_State *getState() const
            {
                return p_L;
            };

            void registerBindings();
            void registerModule(const std::string& name, lua_CFunction loader);
        protected:
            lua_State *p_L;
        };
    } // namespace luau
} // namespace atmo
