#pragma once

#include "lua.h"
#include "luau_ref.hpp"

#include <cstddef>
#include <string>
#include "luau.hpp"

namespace atmo
{
    namespace luau
    {
        class ScriptInstance
        {
        public:
            ScriptInstance(Luau &vm);
            ~ScriptInstance();

            /**
             * @brief
             * Load bytecode inside the script instance
             *
             * @param name The name to give to the loaded chunk
             * @param bytecode The raw bytecode
             * @param size The size of the bytecode
             * @param id The entity id
             * @return true The bytecode was loaded successfully
             * @return false The bytecode couldn't be loaded
             */
            bool load(const std::string &name, const char *bytecode, size_t size, int id);

            /**
             * @brief
             * Call the Create function of the entity (Should be the first Luau function called and only once)
             */
            void create();

            /**
             * @brief 
             * Call the Update function of the entity
             * @param dt The deltatime between this and last call (tick)
             */
            void update(float dt);

            /**
             * @brief
             * Call the PhysiqueUpdate function of the entity
             * @param dt The deltatime between this and last call (tick)
             */
            void physiqueUpdate(float dt);

            /**
             * @brief
             * Mark the script as destroyed therefore no function can be called anymore.
             * Call clean or lose the class reference to clean everything properly
             */
            void destroy();

            /**
             * @brief
             * Clean the script instance properly
             */
            void clean();

            lua_State *getThread() const;

        private:
            Luau &m_vm;

            LuauRef m_envRef;

            lua_State *m_thread = nullptr;
            LuauRef m_threadRef;

            int m_id = -1;

            bool m_stop = false;

            /**
             * @brief
             * Handle the success state of the luau pcall function
             *
             * @param result the result of pcall function
             */
            void handleCall(int result);

            /**
             * @brief
             * Create a luau thread (instance where to run code not a copy of the vm)
             *
             * @param ref The LuauRef class that will hold the reference created for the thread
             * @return lua_State* The new thread created
             */
            lua_State *createThread(LuauRef &ref);

            /**
             * @brief
             * Stores the _G inside the Lua registry so _G can be acessed without relying on the stack later
             *
             * @param thread The thread in which the action will performed
             */
            void createEnvironment(lua_State *thread);
        };
    } // namespace luau
} // namespace atmo
