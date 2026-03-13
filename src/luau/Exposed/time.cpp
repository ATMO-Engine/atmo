#pragma once

#include "luau/instance_manager.hpp"

#include "lua.h"
#include "lualib.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "time.hpp"

namespace atmo
{
    namespace luau
    {

        void Time::RegisterTime(lua_State *state)
        {
            lua_getglobal(state, "_G");

            lua_pushcfunction(state, Time::TimeWait, "Wait");
            lua_setfield(state, -2, "wait");

            lua_pop(state, 1);
        }

        int Time::TimeWait(lua_State *state)
        {
            ScriptInstance *inst = InstanceManager::GetInstance().getScriptInstance(state);

            if (inst == nullptr) {
                spdlog::warn("No script instance for {}", static_cast<void *>(state));
                return 0;
            }

            double t = luaL_checknumber(state, 1);

            inst->setWait(t);

            return lua_yield(state, 0);
        }
    } // namespace luau
} // namespace atmo
