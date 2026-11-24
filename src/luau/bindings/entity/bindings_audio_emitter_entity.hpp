#pragma once
#include <flecs.h>
#include "core/ecs/entities/audio/audio_emitter/audio_emitter.hpp"
#include "lualib.h"
#include "luau/bindings/entity/bindings_entity.hpp"
#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau
{
    template <>
    class LuaBindings<core::ecs::entities::AudioEmitter>
        : public EntityLuaBindings<LuaBindings<core::ecs::entities::AudioEmitter>, core::ecs::entities::AudioEmitter, LuaBindings<core::ecs::entities::Entity>>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, Index, "AudioEmitter.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "AudioEmitter.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, GetAudioPath, "AudioEmitter.getAudioPath");
            lua_setfield(state, -2, "getAudioPath");

            lua_pushcfunction(state, SetAudioPath, "AudioEmitter.setAudioPath");
            lua_setfield(state, -2, "setAudioPath");

            lua_pushcfunction(state, Play, "AudioEmitter.play");
            lua_setfield(state, -2, "play");

            lua_pushcfunction(state, Stop, "AudioEmitter.stop");
            lua_setfield(state, -2, "stop");

            lua_pushcfunction(state, SetPaused, "AudioEmitter.setPaused");
            lua_setfield(state, -2, "setPaused");

            lua_pushcfunction(state, IsPlaying, "AudioEmitter.isPlaying");
            lua_setfield(state, -2, "isPlaying");

            lua_pushcfunction(state, GetVolume, "AudioEmitter.getVolume");
            lua_setfield(state, -2, "getVolume");

            lua_pushcfunction(state, SetVolume, "AudioEmitter.setVolume");
            lua_setfield(state, -2, "setVolume");

            lua_pushcfunction(state, GetPitch, "AudioEmitter.getPitch");
            lua_setfield(state, -2, "getPitch");

            lua_pushcfunction(state, SetPitch, "AudioEmitter.setPitch");
            lua_setfield(state, -2, "setPitch");

            lua_pushcfunction(state, GetPan, "AudioEmitter.getPan");
            lua_setfield(state, -2, "getPan");

            lua_pushcfunction(state, SetPan, "AudioEmitter.setPan");
            lua_setfield(state, -2, "setPan");

            lua_pushcfunction(state, GetLoop, "AudioEmitter.getLoop");
            lua_setfield(state, -2, "getLoop");

            lua_pushcfunction(state, SetLoop, "AudioEmitter.setLoop");
            lua_setfield(state, -2, "setLoop");

            lua_pushcfunction(state, GetAutoplay, "AudioEmitter.getAutoplay");
            lua_setfield(state, -2, "getAutoplay");

            lua_pushcfunction(state, SetAutoplay, "AudioEmitter.setAutoplay");
            lua_setfield(state, -2, "setAutoplay");

            lua_pushcfunction(state, GetBus, "AudioEmitter.getBus");
            lua_setfield(state, -2, "getBus");

            lua_pushcfunction(state, SetBus, "AudioEmitter.setBus");
            lua_setfield(state, -2, "setBus");

            ChainTable(state, LuaBindings<atmo::core::ecs::entities::Entity>::name, "__methods");

            lua_setfield(state, -2, "__methods");

            RegisterProperties(state);

            RegisterIsA(state);

            Seal(state);

            lua_pop(state, 1);

            spdlog::debug("AudioEmitter bindings");
        }

        static Property m_properties[];
        static constexpr const char *name = "AudioEmitter";

    private:
        static int GetAudioPath(lua_State *state);
        static int SetAudioPath(lua_State *state);
        static int Play(lua_State *state);
        static int Stop(lua_State *state);
        static int SetPaused(lua_State *state);
        static int IsPlaying(lua_State *state);
        static int GetVolume(lua_State *state);
        static int SetVolume(lua_State *state);
        static int GetPitch(lua_State *state);
        static int SetPitch(lua_State *state);
        static int GetPan(lua_State *state);
        static int SetPan(lua_State *state);
        static int GetLoop(lua_State *state);
        static int SetLoop(lua_State *state);
        static int GetAutoplay(lua_State *state);
        static int SetAutoplay(lua_State *state);
        static int GetBus(lua_State *state);
        static int SetBus(lua_State *state);
    };
} // namespace atmo::luau
