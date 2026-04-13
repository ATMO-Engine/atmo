#pragma once

#include "luau.hpp"

namespace atmo
{
    namespace luau
    {
        class LuauRef
        {
        public:
            LuauRef() = delete;
            LuauRef(Luau &vm);
            LuauRef(Luau &vm, int ref);
            ~LuauRef();

            /**
             * @brief
             * Store the lua raw ref given so the lifetime will be handled automatically
             *
             * @param ref the lua ref
             */
            void set(int ref);
            void clear();

            int getRef() const;

        private:
            Luau &m_vm;
            int m_ref = LUA_NOREF;
        };
    } // namespace luau
} // namespace atmo
