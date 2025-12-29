#pragma once

#include <string>
#include "lua.h"
#include "luacode.h"
#include "luaconf.h"
#include "lualib.h"

namespace atmo
{
    namespace luau
    {
        class Luau
        {
        public:
            Luau();
            ~Luau();

            static char *Compile(const std::string &source, size_t *bytecode_size, lua_CompileOptions *options = nullptr);
            void runBytecode(const std::string &source, const char *code, size_t size);
            constexpr inline lua_State *getState() const
            {
                return p_L;
            };

        protected:
            lua_State *p_L;
        };
    } // namespace luau
} // namespace atmo
