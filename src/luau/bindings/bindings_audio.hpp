#pragma once
#include "lualib.h"
#include "server/audio/bus.hpp"
#include "spdlog/spdlog.h"

namespace atmo::luau
{
    class AudioBindings
    {
    public:
        static void RegisterType(lua_State *state)
        {
            lua_newtable(state);

            lua_pushcfunction(state, GetBusVolume, "Audio.getBusVolume");
            lua_setfield(state, -2, "getBusVolume");

            lua_pushcfunction(state, SetBusVolume, "Audio.setBusVolume");
            lua_setfield(state, -2, "setBusVolume");

            lua_pushcfunction(state, GetEffect, "Audio.getEffect");
            lua_setfield(state, -2, "getEffect");

            lua_pushcfunction(state, SetEffect, "Audio.setEffect");
            lua_setfield(state, -2, "setEffect");

            lua_pushcfunction(state, RemoveEffect, "Audio.removeEffect");
            lua_setfield(state, -2, "removeEffect");

            lua_pushcfunction(state, SetDucking, "Audio.setDucking");
            lua_setfield(state, -2, "setDucking");

            lua_pushcfunction(state, RemoveDucking, "Audio.removeDucking");
            lua_setfield(state, -2, "removeDucking");

            lua_setglobal(state, "Audio");

            spdlog::debug("Audio bindings");
        }

        /**
         * @brief
         * Read the name of a bus, raises a script error if it is unknown
         *
         * @param state The vm / execution context in which you perform the action
         * @param index the index in the stack for the name
         */
        static server::audio::Bus CheckBus(lua_State *state, int index);

    private:
        static int GetBusVolume(lua_State *state);
        static int SetBusVolume(lua_State *state);
        static int GetEffect(lua_State *state);
        static int SetEffect(lua_State *state);
        static int RemoveEffect(lua_State *state);
        static int SetDucking(lua_State *state);
        static int RemoveDucking(lua_State *state);
    };
} // namespace atmo::luau
