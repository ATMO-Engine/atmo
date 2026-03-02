#include "script_instance.hpp"
#include "instance_manager.hpp"
#include "luau.hpp"
#include "luau_ref.hpp"
#include "lua.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {
        ScriptInstance::ScriptInstance(Luau &vm) : m_vm(vm), m_threadRef(vm) {}

        ScriptInstance::~ScriptInstance()
        {
            clean();
        }

        void ScriptInstance::clean()
        {
            if (m_thread != nullptr) {
                InstanceManager::GetInstance().supressScriptInstance(m_thread);

                m_threadRef.clear();

                m_thread = nullptr;
            }
        }

        void ScriptInstance::createThread()
        {
            lua_State *state = m_vm.getState();

            m_thread = lua_newthread(state);

            int ref = lua_ref(state, LUA_REGISTRYINDEX);
            m_threadRef.set(ref);

            InstanceManager::GetInstance().registerScriptInstance(m_thread, this);
        }

        void ScriptInstance::createEnvironment()
        {
            lua_newtable(m_thread);

            lua_pushinteger(m_thread, m_id);
            lua_setfield(m_thread, -2, "entity");

            lua_newtable(m_thread);
            lua_pushvalue(m_thread, LUA_GLOBALSINDEX);
            lua_setfield(m_thread, -2, "__index");
            lua_setmetatable(m_thread, -2);
        }

        bool ScriptInstance::load(const std::string &name, const char *bytecode, size_t size, int id)
        {
            m_id = id;

            createThread();

            createEnvironment();

            if (!m_vm.LoadBytecodeCoroutine(m_thread, name, bytecode, size, 0)) {
                spdlog::warn("Byte code couldn't be loaded inside thread");
                return false;
            }

            lua_pushvalue(m_thread, -2);   // push env
            lua_setfenv(m_thread, -2);     // set env to chunk

            lua_remove(m_thread, -2);

            int result = lua_resume(m_thread, nullptr, 0);

            if (result != LUA_OK && result != LUA_YIELD)
            {
                const char* err = lua_tostring(m_thread, -1);
                spdlog::warn("Load error: {}", err);
            }

            //if (lua_resume(m_thread, nullptr, 0) != LUA_OK) {
            //    if (lua_status(m_thread) != LUA_YIELD)
            //        {
            //            const char* err = lua_tostring(m_thread, -1);
            //            spdlog::warn("Load error: {}", err);
            //            // TODO: handle runtime error
            //        }
            //}

            return true;
        }

        //bool ScriptInstance::callRef(LuauRef &ref, int nargs)
        //{
        //    if (ref.getRef() == LUA_NOREF) {
        //        spdlog::warn("Ref not found for function"); // TODO: Verbose error
        //        return false;
        //    }
//
        //    lua_rawgeti(m_thread, LUA_REGISTRYINDEX, ref.getRef());
//
        //    if (nargs > 0)
        //        lua_insert(m_thread, -1 - nargs);
//
        //    if (lua_pcall(m_thread, nargs, 0, 0) != LUA_OK)
        //    {
        //        const char* err = lua_tostring(m_thread, -1);
        //        spdlog::error("Lua Runtime Error: {}", err);
        //        lua_pop(m_thread, 1);
        //        return false;
        //    }
        //    return true;
        //}

        void ScriptInstance::handleResume(int result)
        {
            if (result == LUA_YIELD) {
                if (lua_isnumber(m_thread, -1)) {
                    float delay = lua_tonumber(m_thread, -1);
                    //m_wakeTime = m_vm.getTime() + delay;
                }

                lua_pop(m_thread, 1);

            } else if (result == LUA_OK) {
                // Succes
            } else {
                const char* err = lua_tostring(m_thread, -1);
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

            if (m_thread == nullptr) {
                spdlog::warn("Thread null, code not running");
            }

            lua_pushnumber(m_thread, dt);
            int result = lua_resume(m_thread, nullptr, 1);
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
            return m_thread;
        }
    } // namespace luau
} // namespace atmo
