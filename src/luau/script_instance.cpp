#include "script_instance.hpp"
#include "instance_manager.hpp"
#include "lua.h"
#include "lualib.h"
#include "luau.hpp"
#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau_ref.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {
        ScriptInstance::ScriptInstance(Luau *vm) : m_vm(vm), m_envRef(vm), m_threadRef(vm) {}

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

        lua_State *ScriptInstance::createThread(LuauRef &ref)
        {
            if (!m_vm) {
                spdlog::error("ScriptInstance: vm is null, cannot create thread");
                return nullptr;
            }

            lua_State *state = m_vm->getState();

            lua_State *newThread = lua_newthread(state);

            ref.set(lua_ref(state, -1));
            lua_pop(state, 1);

            // not used now, but will be usefull if we ever need the code to get the script instance
            // (example asynchonous code to stop and resume the right instance for a wait(x) function)
            InstanceManager::GetInstance().registerScriptInstance(newThread, this);

            return newThread;
        }

        void ScriptInstance::createEnvironment(lua_State *thread)
        {
            lua_pushvalue(thread, LUA_GLOBALSINDEX);

            m_envRef.set(lua_ref(thread, -1));
            lua_pop(thread, 1);
        }

        bool ScriptInstance::pushTypeEntity(lua_State *L, flecs::entity &e)
        {
            return luau::LuauRegistry::Instance().pushEntity(L, e);
        }

        bool ScriptInstance::load(const std::string &name, const char *bytecode, size_t size, flecs::entity &entity)
        {
            clean();
            m_envRef.clear();
            m_stop = false;

            m_thread = createThread(m_threadRef);
            if (m_thread == nullptr) {
                m_stop = true;
                return false;
            }
            luaL_sandboxthread(m_thread);
            createEnvironment(m_thread);

            if (!pushTypeEntity(m_thread, entity)) {
                return failLoad();
            }
            lua_setglobal(m_thread, "this");

            if (!m_vm->LoadBytecodeCoroutine(m_thread, name, bytecode, size, 0)) {
                spdlog::warn("Byte code couldn't be loaded inside thread");
                return failLoad();
            }

            if (!Luau::ProtectedCall(m_thread, 0, name)) {
                return failLoad();
            }

            return true;
        }

        bool ScriptInstance::failLoad()
        {
            clean();
            m_envRef.clear();
            m_stop = true;
            return false;
        }

        void ScriptInstance::create()
        {
            if (m_stop == true) {
                return;
            }
            if (m_thread == nullptr) {
                spdlog::warn("Thread null, code not running");
                return;
            }

            if (!pushFunction("Create")) {
                return;
            }
            Luau::ProtectedCall(m_thread, 0, "Create");
        }

        void ScriptInstance::update(float dt)
        {
            if (m_stop == true) {
                return;
            }

            if (m_thread == nullptr) {
                spdlog::warn("Thread null, code not running");
                return;
            }

            if (!pushFunction("Update")) {
                return;
            }
            lua_pushnumber(m_thread, dt);
            Luau::ProtectedCall(m_thread, 1, "Update");
        }

        void ScriptInstance::physicsUpdate(float dt)
        {
            if (m_stop == true) {
                return;
            }

            if (m_thread == nullptr) {
                spdlog::warn("Thread null, code not running");
                return;
            }

            if (!pushFunction("PhysicsUpdate")) {
                return;
            }
            lua_pushnumber(m_thread, dt);
            Luau::ProtectedCall(m_thread, 1, "PhysicsUpdate");
        }

        bool ScriptInstance::pushFunction(const char *name)
        {
            lua_getglobal(m_thread, name);
            if (!lua_isfunction(m_thread, -1)) {
                lua_pop(m_thread, 1);
                return false;
            }
            return true;
        }

        void ScriptInstance::onCollisionEnter(flecs::entity &other)
        {
            if (m_stop == true) {
                return;
            }
            if (m_thread == nullptr) {
                spdlog::warn("Thread null, code not running");
                return;
            }

            if (!pushFunction("OnCollisionEnter")) {
                return;
            }

            if (!pushTypeEntity(m_thread, other)) {
                lua_pop(m_thread, 1);
                return;
            }

            Luau::ProtectedCall(m_thread, 1, "OnCollisionEnter");
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
