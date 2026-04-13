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

            /**
             * @brief
             * Add a ScriptInstance into a map so the class reference can be retrieve later through the luau thread
             *
             * @param thread The lua thread associated to the ScriptInstance class
             * @param inst the ScriptInstance class
             */
            void registerScriptInstance(lua_State *thread, ScriptInstance *inst);

            /**
             * @brief
             * Get the ScriptInstance reference attached to the thread
             *
             * @param thread The lua thread retrieve through Luau function call
             * @return ScriptInstance* The ScriptInstance reference
             */
            ScriptInstance *getScriptInstance(lua_State *thread) const;

            /**
             * @brief
             * Remove a ScriptInstance ref from the map example (entity supressed => ScriptInstance supressed => removed from the map)
             *
             * @param thread The thread which was associated with the ScriptInstance you want to remove
             */
            void supressScriptInstance(lua_State *thread);

        private:
            InstanceManager();
            ~InstanceManager() = default;

            std::unordered_map<lua_State *, ScriptInstance *> m_scriptInstances;
        };
    } // namespace luau
} // namespace atmo
