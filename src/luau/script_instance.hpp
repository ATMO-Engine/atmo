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

            void update(float dt);
            void destroy();

            void clean();

            lua_State *getThread() const;

            void setWait(double timeSecond);

        private:
            Luau &m_vm;

            lua_State *m_thread = nullptr;
            LuauRef m_threadRef;

            int m_id = -1;

            bool m_stop = false;

            bool m_sleeping = false;
            double m_sleepTime = 0;

            void handleResume(int result);

            void createThread();
            void createEnvironment();
        };
    } // namespace luau
} // namespace atmo
