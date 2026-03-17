#include "script_instance.hpp"
#include "instance_manager.hpp"
#include "lua.h"
#include "lualib.h"
#include "luau.hpp"
#include "luau_ref.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {
        ScriptInstance::ScriptInstance(Luau &vm) : m_vm(vm), m_envRef(vm), m_updateThreadRef(vm), m_physiqueThreadRef(vm) {}

        ScriptInstance::~ScriptInstance()
        {
            clean();
        }

        void ScriptInstance::clean()
        {
            if (m_updateThread != nullptr) {
                InstanceManager::GetInstance().supressScriptInstance(m_updateThread);

                m_updateThreadRef.clear();

                m_updateThread = nullptr;
            }
            if (m_physiqueThread != nullptr) {
                InstanceManager::GetInstance().supressScriptInstance(m_physiqueThread);

                m_physiqueThreadRef.clear();

                m_physiqueThread = nullptr;
            }
        }

        lua_State *ScriptInstance::createThread(LuauRef &ref)
        {
            lua_State *state = m_vm.getState();

            lua_State *newThread = lua_newthread(state);

            int r = lua_ref(state, LUA_REGISTRYINDEX);
            ref.set(r);

            InstanceManager::GetInstance().registerScriptInstance(newThread, this);

            return newThread;
        }

        void ScriptInstance::createEnvironment()
        {
            lua_newtable(m_updateThread);

            lua_pushinteger(m_updateThread, m_id);
            lua_setfield(m_updateThread, -2, "entity");

            lua_newtable(m_updateThread);
            lua_pushvalue(m_updateThread, LUA_GLOBALSINDEX);
            lua_setfield(m_updateThread, -2, "__index");
            lua_setmetatable(m_updateThread, -2);
        }

        bool ScriptInstance::load(const std::string &name, const char *bytecode, size_t size, int id)
        {
            m_id = id;

            m_updateThread = createThread(m_updateThreadRef);
            m_physiqueThread = createThread(m_physiqueThreadRef);

            //createEnvironment();
lua_newtable(m_updateThread);            // push new table as env
int envIndex = lua_gettop(m_updateThread);

lua_newtable(m_updateThread);            // metatable
lua_getglobal(m_updateThread, "_G");     // push _G from VM
lua_setfield(m_updateThread, -2, "__index");  // mt.__index = _G
lua_setmetatable(m_updateThread, envIndex);   // set metatable for env table

// 3️⃣ store a reference in the registry so you can reuse it
int ref = lua_ref(m_updateThread, LUA_REGISTRYINDEX);  // stores env table ref in ScriptInstance
m_envRef.set(ref);

            if (!m_vm.LoadBytecodeCoroutine(m_updateThread, name, bytecode, size, envIndex)) {
                spdlog::warn("Byte code couldn't be loaded inside thread");
                return false;
            }


            luaL_sandboxthread(m_updateThread);



            lua_rawgeti(m_physiqueThread, LUA_REGISTRYINDEX, m_envRef.getRef());
            int sharedindex = lua_gettop(m_physiqueThread);
            
            lua_pushvalue(m_updateThread, -1);
            lua_xmove(m_updateThread, m_physiqueThread, 1);

            
            int result = lua_resume(m_updateThread, nullptr, 0);

            if (result != LUA_OK && result != LUA_YIELD) {
                const char *err = lua_tostring(m_updateThread, -1);
                spdlog::warn("Load error: {}", err);
            }


            result = lua_resume(m_physiqueThread, nullptr, 0);

            if (result != LUA_OK && result != LUA_YIELD) {
                const char *err = lua_tostring(m_physiqueThread, -1);
                spdlog::warn("Load error: {}", err);
            }


            return true;
        }

        void ScriptInstance::handleResume(int result)
        {
            if (result == LUA_YIELD) {
                if (lua_isnumber(m_physiqueThread, -1)) {
                    float delay = lua_tonumber(m_physiqueThread, -1);
                    // m_wakeTime = m_vm.getTime() + delay;
                }

                lua_pop(m_physiqueThread, 1);

            } else if (result == LUA_OK) {
                // Succes
            } else {
                const char *err = lua_tostring(m_physiqueThread, -1);
                spdlog::warn("Run time error: {}", err);
            }
        }

        void ScriptInstance::update(float dt)
        {
            if (m_stop == true) {
                return;
            }

            if (m_sleepTime > 0) {
                m_sleepTime -= dt;
                return;
            }

            if (m_physiqueThread == nullptr) {
                spdlog::warn("Thread null, code not running");
            }

            lua_pushnumber(m_physiqueThread, dt);
            int result = lua_resume(m_physiqueThread, nullptr, 1);
            handleResume(result);
        }

        void ScriptInstance::setWait(double timeSecond)
        {
            m_sleeping = true;
            m_sleepTime = timeSecond;
        }

        void ScriptInstance::destroy()
        {
            m_stop = true;
        }

        lua_State *ScriptInstance::getThread() const
        {
            return m_physiqueThread;
        }
    } // namespace luau
} // namespace atmo
