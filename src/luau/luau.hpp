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
            static char *Compile(const std::string &source, size_t *bytecode_size, const std::string &chunkName, lua_CompileOptions *options = nullptr);

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
            ScriptInstance *generateInstance();

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

            /**
             * @brief
             * Call the function on the stack in protected mode, every error is logged with its traceback.
             * Stack: [function, arg1 ... argN] -> [] (no result is kept)
             *
             * @param L The vm or thread you are working on
             * @param nargs The number of arguments pushed after the function
             * @param context The context of the call, shown in the log (ex: script or callback name)
             * @return true if the call succeeded, false if it raised an error (already logged)
             */
            static bool ProtectedCall(lua_State *L, int nargs, const std::string &context);

            /**
             * @brief
             * Send log error message, pop the error message from the stack
             *
             * @param L The vm or thread you are working on
             * @param context The context of the environment (ex: script or entity name)
             * @param traceback The call stack at the moment of the error, empty if unknown
             */
            static void LogLuauError(lua_State *L, const std::string &context, const std::string &traceback = "");

            /**
             * @brief
             * Send log error message
             *
             * @param errorMsg The generated error message
             * @param loader The context of the environment (ex: script or entity name)
             */
            static void LogCompileTimeError(const std::string &errorMsg, const std::string &context);

            static Luau &Instance()
            {
                static Luau instance;
                return instance;
            };

        protected:
            lua_State *p_L;
        };
    } // namespace luau
} // namespace atmo
