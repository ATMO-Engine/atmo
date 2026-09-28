#pragma once

#include "lua.h"
#include "luau_ref.hpp"
#include "luau.hpp"

#include "flecs.h"

#include <cstddef>
#include <string>


namespace atmo
{
    namespace luau
    {
        class ScriptInstance
        {
        public:
            ScriptInstance(Luau *vm);
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
            bool load(const std::string &name, const char *bytecode, size_t size, flecs::entity &entity);

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
             * Call the physicsUpdate function of the entity
             * @param dt The deltatime between this and last call (tick)
             */
            void physicsUpdate(float dt);

            /**
             * @brief
             * Call the function OnCollisionEnter of the script if defined
             * @param other The other entity of the colision
             */
            void onCollisionEnter(flecs::entity &other);

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
            Luau *m_vm = nullptr;

            LuauRef m_envRef;

            lua_State *m_thread = nullptr;
            LuauRef m_threadRef;

            bool m_stop = false;

            /**
             * @brief Push a global function on the stack if it exists (nothing is pushed otherwise)
             * @param name The name of the function to push
             * @return true if function has been found and push, int other case false
             */
            bool pushFunction(const char *name);

            /**
             * @brief
             * Release everything created by a failed load and stop the instance so no callback is called
             *
             * @return false always, to be returned by load
             */
            bool failLoad();

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

            /**
             * @brief
             * Register the entity given to the lua stack and attach the metatable attached to the entity type
             *
             * @param L The vm in which it is registered
             * @param e the entity to register
             * @return true if the metatable for the given type exist
             * @return false if it doesn't exist
             */
            bool pushTypeEntity(lua_State *L, flecs::entity &e);
        };
    } // namespace luau
} // namespace atmo
