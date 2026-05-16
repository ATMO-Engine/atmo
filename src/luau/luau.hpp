#pragma once

#include <cstddef>
#include <string>
#include "lua.h"
#include "luacode.h"

namespace atmo
{
    namespace luau
    {
        class ScriptInstance;


        class Luau
        {
        public:
            Luau();
            ~Luau();

            /**
             * @brief
             * Compile luau file into raw bytecode
             *
             * @param source The path of the luau file
             * @param bytecode_size The size of the Luau file
             * @param options Control how the code is compiled example optimization/debug level
             * @return char* The compiled raw bytecode
             */
            static char *Compile(const std::string &source, size_t *bytecode_size, lua_CompileOptions *options = nullptr);

            /**
             * @brief
             * Load bytecode into the vm, then becoming callable function on the luau stack
             *
             * @param name The name to give to the loaded chunk
             * @param code The raw bytecode
             * @param size The size of the bytecode
             * @param env Environment table to use (default = 0)
             * @return true The bytecode was loaded successfully
             * @return false The bytecode couldn't be loaded
             */
            bool loadBytecode(const std::string &name, const char *code, size_t size, int env = 0);

            /**
             * @brief
             * Load bytecode into lua_thread (instance of execution), then becoming callable
             *  function on the luau stack
             *
             * @param coroutine The instance in which you want the code to be loaded
             * @param name The name to give to the loaded chunk
             * @param code The raw bytecode
             * @param size The size of the bytecode
             * @param env Environment table to use (default = 0)
             * @return true The bytecode was loaded successfully
             * @return false The bytecode couldn't be loaded
             */
            static bool LoadBytecodeCoroutine(lua_State *coroutine, const std::string &name, const char *code, size_t size, int env = 0);

            /**
             * @brief
             * Generate an instance of script to attach to a component
             *
             * @return ScriptInstance The instance generated
             */
            ScriptInstance generateInstance();

            constexpr inline lua_State *getState() const
            {
                return p_L;
            };

            /**
             * @brief
             * Load the exposed C++ code inside the luau vm
             */
            void registerBindings();

            /**
             * @brief
             * Load a module inside the Luau vm
             *
             * @param name The name to give to the laoded module
             * @param loader The function that load the module
             */
            void registerModule(const std::string &name, lua_CFunction loader);

        protected:
            lua_State *p_L;
        };
    } // namespace luau
} // namespace atmo
