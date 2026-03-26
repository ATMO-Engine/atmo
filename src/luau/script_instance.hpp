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

            bool load(const std::string &name, const char *bytecode, size_t size, int id);

            void create();
            void update(float dt);
            void physiqueUpdate(float dt);
            void destroy();

            void clean();

            lua_State *getThread() const;

        private:
            Luau &m_vm;

            LuauRef m_envRef;

            lua_State *m_thread = nullptr;
            LuauRef m_threadRef;

            int m_id = -1;

            bool m_stop = false;

            void handleCall(int result);

            lua_State *createThread(LuauRef &ref);
            void createEnvironment(lua_State *thread);
            void copyEnv(lua_State *from, lua_State *to);
        };
    } // namespace luau
} // namespace atmo
