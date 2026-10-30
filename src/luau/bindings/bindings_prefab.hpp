#pragma once
#include "lualib.h"
#include "spdlog/spdlog.h"

namespace atmo::luau
{
    class PrefabBindings
    {
    public:
        static void RegisterType(lua_State *state)
        {
            lua_newtable(state);

            lua_pushcfunction(state, Instantiate, "Prefab.instantiate");
            lua_setfield(state, -2, "instantiate");

            lua_setglobal(state, "Prefab");

            spdlog::debug("Prefab bindings");
        }

    private:
        /**
         * @brief Prefab.instantiate(path, parent, callback?)
         * Creation is queued until the end of the frame, once the world can be modified.
         * The optional callback is then called with the prefab root.
         */
        static int Instantiate(lua_State *state);
    };
} // namespace atmo::luau
