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

            static char *compile(const std::string &source, size_t *bytecode_size,
                                 lua_CompileOptions *options = nullptr);
            void run_bytecode(const std::string &source, const char *code, size_t size);

        protected:
            lua_State *L;
        };
    } // namespace luau
} // namespace atmo
