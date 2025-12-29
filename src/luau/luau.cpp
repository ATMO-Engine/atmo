#include "luau.hpp"
#include "luacode.h"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {
        Luau::Luau()
        {
            p_L = luaL_newstate();
            luaL_openlibs(p_L);
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

        void Luau::runBytecode(const std::string &source, const char *code, size_t size)
        {
            const int result = luau_load(p_L, source.c_str(), code, size, 0);
            if (result != 0) {
                spdlog::error("Failed to load Lua code:" + source);
            }
        }
    } // namespace luau

} // namespace atmo
