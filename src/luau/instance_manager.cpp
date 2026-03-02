#include "instance_manager.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {

        InstanceManager::InstanceManager() {}


        InstanceManager &InstanceManager::GetInstance()
        {
            static InstanceManager instance;
            return instance;
        }

        void InstanceManager::registerScriptInstance(lua_State *thread, ScriptInstance *inst)
        {
            if (thread == nullptr || inst == nullptr) {
                spdlog::warn("Thread (coroutine) {} or Script instance {} is null", static_cast<void *>(thread), static_cast<void *>(inst));
                return;
            }
            m_scriptInstances.insert(std::make_pair(thread, inst));
        }

        ScriptInstance *InstanceManager::getScriptInstance(lua_State *thread) const
        {
            if (m_scriptInstances.find(thread) != m_scriptInstances.end()) {
                return m_scriptInstances.at(thread);
            }
            spdlog::warn("No script instance registered for thread {}", static_cast<void *>(thread));
            return nullptr;
        }

        void InstanceManager::supressScriptInstance(lua_State *thread)
        {
            auto it = m_scriptInstances.find(thread);
            if (it != m_scriptInstances.end()) {
                m_scriptInstances.erase(it);
            } else {
                spdlog::warn("No script instance registered for thread {}, supress ignored", static_cast<void *>(thread));
            }
        }
    } // namespace luau
} // namespace atmo
