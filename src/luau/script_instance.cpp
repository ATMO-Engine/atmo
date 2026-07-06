#include "script_instance.hpp"
#include <iostream>
#include "instance_manager.hpp"
#include "lua.h"
#include "lualib.h"
#include "luau.hpp"
#include "luau_ref.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"


#include "bindings/bindings_entity.hpp"


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

            int r = lua_ref(state, -1);
            ref.set(r);

            // not used now, but will be usefull if we ever need the code to get the script instance
            // (example asynchonous code to stop and resume the right instance for a wait(x) function)
            InstanceManager::GetInstance().registerScriptInstance(newThread, this);

            return newThread;
        }

        void ScriptInstance::createEnvironment(lua_State *thread)
        {
            lua_pushvalue(thread, LUA_GLOBALSINDEX);

            int ref = lua_ref(thread, -1);
            m_envRef.set(ref);
        }

        bool ScriptInstance::load(const std::string &name, const char *bytecode, size_t size, flecs::entity &entity)
        {
            m_thread = createThread(m_threadRef);
            if (m_thread == nullptr) {
                return false;
            }

            luaL_sandboxthread(m_thread);

            createEnvironment(m_thread);

            void *mem = lua_newuserdata(m_thread, sizeof(std::shared_ptr<flecs::entity>));
            new (mem) std::shared_ptr<flecs::entity>(std::make_shared<flecs::entity>(entity));
            luaL_getmetatable(m_thread, LuaBindings<flecs::entity>::name);
            lua_setmetatable(m_thread, -2);
            lua_setglobal(m_thread, "this");

            if (!m_vm) {
                spdlog::error("ScriptInstance: vm is null, cannot load bytecode");
                return false;
            }

            if (!m_vm->LoadBytecodeCoroutine(m_thread, name, bytecode, size, 0)) {
                spdlog::warn("Byte code couldn't be loaded inside thread");
                return false;
            }

            int result = lua_pcall(m_thread, 0, 0, 0);
            if (result != LUA_OK) {
                const char *err = lua_tostring(m_thread, -1);
                spdlog::warn("Load error: {}", err);
            }

            return true;
        }

        void ScriptInstance::handleCall(int result)
        {
            if (result != LUA_OK) {
                luau::Luau::LogLuauError(m_thread, "runtime");
            }
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

            lua_getglobal(m_thread, "Create");
            int result = lua_pcall(m_thread, 0, 0, 0);
            handleCall(result);
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

            lua_getglobal(m_thread, "Update");
            lua_pushnumber(m_thread, dt);
            int result = lua_pcall(m_thread, 1, 0, 0);
            handleCall(result);
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

            lua_getglobal(m_thread, "PhysicsUpdate");
            lua_pushnumber(m_thread, dt);
            int result = lua_pcall(m_thread, 1, 0, 0);
            handleCall(result);
        }

        bool ScriptInstance::pushFunction(const char *name)
        {
            lua_getglobal(m_thread, name);
            if (!lua_isfunction(m_thread, -1)) {
                lua_pop(m_thread, 1);
                spdlog::warn("function {} is not defined", name);
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

            LuaBindings<flecs::entity>::Push(m_thread, &other, false);

            int result = lua_pcall(m_thread, 1, 0, 0);
            handleCall(result);
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
