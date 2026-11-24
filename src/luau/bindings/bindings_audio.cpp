#include "bindings_audio.hpp"

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "server/audio/audio_server.hpp"

namespace atmo::luau
{
    namespace
    {
        void AppendName(std::string &list, std::string_view name)
        {
            if (!list.empty())
                list += ", ";
            list += name;
        }

        server::audio::EffectKind CheckEffect(lua_State *state, int index)
        {
            const auto kind = server::audio::ParseEffect(luaL_checkstring(state, index));
            if (!kind) {
                std::string names;
                for (const std::string_view name : server::audio::effect_names) AppendName(names, name);
                luaL_argerror(state, index, ("expected one of: " + names).c_str());
            }
            return *kind;
        }

        // Reads an optional table of settings, such as { cutoff = 400 }
        std::vector<server::audio::EffectValue> CheckValues(lua_State *state, int index, std::span<const server::audio::EffectParameter> parameters)
        {
            std::vector<server::audio::EffectValue> values;
            if (lua_isnoneornil(state, index))
                return values;
            luaL_checktype(state, index, LUA_TTABLE);

            lua_pushnil(state);
            while (lua_next(state, index) != 0) {
                const std::string_view key = lua_type(state, -2) == LUA_TSTRING ? lua_tostring(state, -2) : "";
                const auto it = std::ranges::find(parameters, key, &server::audio::EffectParameter::name);
                if (it == parameters.end()) {
                    std::string names;
                    for (const auto &parameter : parameters) AppendName(names, parameter.name);
                    luaL_argerror(state, index, ("expected settings among: " + names).c_str());
                }
                if (lua_type(state, -1) != LUA_TNUMBER)
                    luaL_argerror(state, index, (std::string(it->name) + " must be a number").c_str());
                values.push_back({ it->name, static_cast<float>(lua_tonumber(state, -1)) });
                lua_pop(state, 1);
            }
            return values;
        }
    } // namespace

    server::audio::Bus AudioBindings::CheckBus(lua_State *state, int index)
    {
        const auto bus = server::audio::ParseBus(luaL_checkstring(state, index));
        if (!bus)
            luaL_argerror(state, index, "expected master, music or sfx");
        return *bus;
    }

    int AudioBindings::GetBusVolume(lua_State *state)
    {
        lua_pushnumber(state, server::audio::AudioServer::Instance().busGetVolume(CheckBus(state, 1)));
        return 1;
    }

    int AudioBindings::SetBusVolume(lua_State *state)
    {
        const auto bus = CheckBus(state, 1);
        const float volume = static_cast<float>(luaL_checknumber(state, 2));
        server::audio::AudioServer::Instance().busSetVolume(bus, volume);
        return 0;
    }

    int AudioBindings::GetEffect(lua_State *state)
    {
        const auto bus = CheckBus(state, 1);
        const auto effect = server::audio::AudioServer::Instance().busGetEffect(bus, CheckEffect(state, 2));
        if (!effect) {
            lua_pushnil(state);
            return 1;
        }

        lua_createtable(state, 0, static_cast<int>(effect->size()));
        for (const auto &setting : *effect) {
            lua_pushlstring(state, setting.name.data(), setting.name.size());
            lua_pushnumber(state, setting.value);
            lua_settable(state, -3);
        }
        return 1;
    }

    int AudioBindings::SetEffect(lua_State *state)
    {
        const auto bus = CheckBus(state, 1);
        const auto kind = CheckEffect(state, 2);
        const auto values = CheckValues(state, 3, server::audio::EffectParameters(kind));
        server::audio::AudioServer::Instance().busSetEffect(bus, kind, values);
        return 0;
    }

    int AudioBindings::RemoveEffect(lua_State *state)
    {
        const auto bus = CheckBus(state, 1);
        server::audio::AudioServer::Instance().busRemoveEffect(bus, CheckEffect(state, 2));
        return 0;
    }

    int AudioBindings::SetDucking(lua_State *state)
    {
        const auto bus = CheckBus(state, 1);
        const auto trigger = CheckBus(state, 2);
        if (bus == server::audio::Bus::Master)
            luaL_argerror(state, 1, "master cannot be ducked");
        if (trigger == server::audio::Bus::Master || trigger == bus)
            luaL_argerror(state, 2, "expected another bus than master and the ducked one");
        const auto values = CheckValues(state, 3, server::audio::Ducker::parameters);
        server::audio::AudioServer::Instance().busSetDucking(bus, trigger, values);
        return 0;
    }

    int AudioBindings::RemoveDucking(lua_State *state)
    {
        server::audio::AudioServer::Instance().busRemoveDucking(CheckBus(state, 1));
        return 0;
    }
} // namespace atmo::luau
