#include "luau.hpp"
#include "luacode.h"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {
        Luau::Luau()
        {
            L = luaL_newstate();
            luaL_openlibs(L);
        }

        Luau::~Luau()
        {
            if (L)
                lua_close(L);
            L = nullptr;
        }

        char *Luau::compile(const std::string &source, size_t *bytecode_size, lua_CompileOptions *options)
        {
            return luau_compile(source.c_str(), source.size(), options, bytecode_size);
        }

        void Luau::run_bytecode(const std::string &source, const char *code, size_t size)
        {
            const int result = luau_load(L, source.c_str(), code, size, 0);
            if (result != 0) {
                spdlog::error("Failed to load Lua code:" + source);
            }
        }
    } // namespace luau

} // namespace atmo
