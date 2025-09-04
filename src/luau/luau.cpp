#include "luau.hpp"
#include "lualib.h"

namespace atmo
{
    namespace luau
    {
        void Luau::init()
        {
            L = luaL_newstate();
            luaL_openlibs(L);
        }

        void Luau::close()
        {
            if (L) {
                lua_close(L);
                L = nullptr;
            }
        }
    } // namespace luau

} // namespace atmo
