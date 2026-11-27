#include "bindings_audio_emitter_entity.hpp"

#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau/bindings/bindings_audio.hpp"

namespace atmo::luau
{
    namespace
    {
        const core::components::AudioEmitter defaults;

        core::ecs::entities::AudioEmitter *AliveEmitter(lua_State *state)
        {
            auto *entity = LuaBindings<core::ecs::entities::AudioEmitter>::CheckPtr(state, 1);
            if (!entity || !entity->getHandle().is_alive() || !entity->getHandle().has<core::components::AudioEmitter>()) {
                spdlog::warn("AudioEmitter is not alive");
                return nullptr;
            }
            return entity;
        }
    } // namespace

    Property LuaBindings<core::ecs::entities::AudioEmitter>::m_properties[] = { { nullptr, nullptr, nullptr } };

    int LuaBindings<core::ecs::entities::AudioEmitter>::GetAudioPath(lua_State *state)
    {
        auto *emitter = AliveEmitter(state);
        const std::string_view path = emitter ? emitter->getAudioPath() : std::string_view{ defaults.audio_path };
        lua_pushlstring(state, path.data(), path.size());
        return 1;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::SetAudioPath(lua_State *state)
    {
        const char *path = luaL_optstring(state, 2, "");
        if (auto *emitter = AliveEmitter(state))
            emitter->setAudioPath(path);
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::Play(lua_State *state)
    {
        if (auto *emitter = AliveEmitter(state))
            emitter->play();
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::Stop(lua_State *state)
    {
        if (auto *emitter = AliveEmitter(state))
            emitter->stop();
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::SetPaused(lua_State *state)
    {
        const bool paused = luaL_checkboolean(state, 2);
        if (auto *emitter = AliveEmitter(state))
            emitter->setPaused(paused);
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::IsPlaying(lua_State *state)
    {
        auto *emitter = AliveEmitter(state);
        lua_pushboolean(state, emitter && emitter->isPlaying());
        return 1;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::GetVolume(lua_State *state)
    {
        auto *emitter = AliveEmitter(state);
        lua_pushnumber(state, emitter ? emitter->getVolume() : defaults.volume);
        return 1;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::SetVolume(lua_State *state)
    {
        const float volume = static_cast<float>(luaL_checknumber(state, 2));
        if (auto *emitter = AliveEmitter(state))
            emitter->setVolume(volume);
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::GetPitch(lua_State *state)
    {
        auto *emitter = AliveEmitter(state);
        lua_pushnumber(state, emitter ? emitter->getPitch() : defaults.pitch);
        return 1;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::SetPitch(lua_State *state)
    {
        const float pitch = static_cast<float>(luaL_checknumber(state, 2));
        if (auto *emitter = AliveEmitter(state))
            emitter->setPitch(pitch);
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::GetPan(lua_State *state)
    {
        auto *emitter = AliveEmitter(state);
        lua_pushnumber(state, emitter ? emitter->getPan() : defaults.pan);
        return 1;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::SetPan(lua_State *state)
    {
        const float pan = static_cast<float>(luaL_checknumber(state, 2));
        if (auto *emitter = AliveEmitter(state))
            emitter->setPan(pan);
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::GetLoop(lua_State *state)
    {
        auto *emitter = AliveEmitter(state);
        lua_pushboolean(state, emitter ? emitter->getLoop() : defaults.loop);
        return 1;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::SetLoop(lua_State *state)
    {
        const bool loop = luaL_checkboolean(state, 2);
        if (auto *emitter = AliveEmitter(state))
            emitter->setLoop(loop);
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::GetAutoplay(lua_State *state)
    {
        auto *emitter = AliveEmitter(state);
        lua_pushboolean(state, emitter ? emitter->getAutoplay() : defaults.autoplay);
        return 1;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::SetAutoplay(lua_State *state)
    {
        const bool autoplay = luaL_checkboolean(state, 2);
        if (auto *emitter = AliveEmitter(state))
            emitter->setAutoplay(autoplay);
        return 0;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::GetBus(lua_State *state)
    {
        auto *emitter = AliveEmitter(state);
        const std::string_view bus = server::audio::BusName(emitter ? emitter->getBus() : server::audio::Bus::Master);
        lua_pushlstring(state, bus.data(), bus.size());
        return 1;
    }

    int LuaBindings<core::ecs::entities::AudioEmitter>::SetBus(lua_State *state)
    {
        const auto bus = AudioBindings::CheckBus(state, 2);
        if (auto *emitter = AliveEmitter(state))
            emitter->setBus(bus);
        return 0;
    }
} // namespace atmo::luau

ATMO_REGISTER_LUA_ENTITY_BINDING(atmo::core::ecs::entities::AudioEmitter);
