#pragma once

#include "luau_ref.hpp"
#include "lua.h"
#include "luau.hpp"

namespace atmo
{
    namespace luau
    {
        LuauRef::LuauRef(Luau &vm) : m_vm(vm) {}

        LuauRef::LuauRef(Luau &vm, int ref) : m_vm(vm)
        {
            set(ref);
        }

        LuauRef::~LuauRef()
        {
            clear();
        }

        void LuauRef::set(int ref)
        {
            m_ref = ref;
        }

        void LuauRef::clear()
        {
            lua_State *state = m_vm.getState();
            if (state && m_ref != LUA_NOREF)
            {
                lua_pushnil(state);
                lua_rawseti(state, LUA_REGISTRYINDEX, m_ref);
                m_ref = LUA_NOREF;
            }
        }

        int LuauRef::getRef() const
        {
            return m_ref;
        }
    } // namespace luau
} // namespace atmo
