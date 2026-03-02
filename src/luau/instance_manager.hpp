#pragma once

#include "script_instance.hpp"

#include "lua.h"

#include <unordered_map>

namespace atmo
{
    namespace luau
    {
        class InstanceManager
        {
        public:
            static InstanceManager &GetInstance();

            void registerScriptInstance(lua_State *thread, ScriptInstance *inst);

            ScriptInstance *getScriptInstance(lua_State *thread) const;

            void supressScriptInstance(lua_State *thread);
        private:
            InstanceManager();
            ~InstanceManager() = default;

            std::unordered_map<lua_State *, ScriptInstance *> m_scriptInstances;
    };
    } // namespace luau
} // namespace atmo
