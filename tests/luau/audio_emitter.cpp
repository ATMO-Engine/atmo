#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "core/ecs/components.hpp"
#include "core/ecs/entities/audio/audio_emitter/audio_emitter.hpp"
#include "core/ecs/entities/script.hpp"
#include "core/ecs/entity_registry.hpp"
#include "lua.h"
#include "lualib.h"
#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau/bindings/entity/bindings_audio_emitter_entity.hpp"
#include "luau/luau.hpp"
#include "server/audio/audio_server.hpp"

using atmo::core::ecs::EntityRegistry;
using atmo::luau::Luau;
using atmo::luau::LuauRegistry;
using Emitter = atmo::core::ecs::entities::AudioEmitter;

namespace
{
    struct ScopedBuses {
        ScopedBuses() = default;
        ~ScopedBuses()
        {
            using atmo::server::audio::Bus;
            auto &server = atmo::server::audio::AudioServer::Instance();
            for (const Bus bus : { Bus::Master, Bus::Music, Bus::Sfx }) {
                server.busSetVolume(bus, 1.0f);
                server.busRemoveDucking(bus);
                for (const auto name : atmo::server::audio::effect_names) server.busRemoveEffect(bus, *atmo::server::audio::ParseEffect(name));
            }
        }

        ScopedBuses(const ScopedBuses &) = delete;
        ScopedBuses &operator=(const ScopedBuses &) = delete;
    };

    struct ScriptWorld {
        std::filesystem::path root = std::filesystem::temp_directory_path() / "atmo-audio-luau-test";
        flecs::world world;
        std::shared_ptr<Emitter> emitter;

        ScriptWorld()
        {
            std::filesystem::remove_all(root);
            std::filesystem::create_directories(root);
            atmo::core::components::register_core_components(world);
            atmo::core::ecs::entities::Entity::RegisterSystems(&world);
            Emitter::RegisterSystems(&world);
            emitter = EntityRegistry::CreateIn<Emitter>(&world, "Entity::AudioEmitter");
            REQUIRE(emitter != nullptr);
        }

        ~ScriptWorld()
        {
            std::error_code error;
            std::filesystem::remove_all(root, error);
        }

        std::string write(const std::string &name, const std::string &content) const
        {
            std::ofstream out(root / name, std::ios::binary | std::ios::trunc);
            out << content;
            return (root / name).generic_string();
        }

        std::string wav() const
        {
            const auto append = [](std::string &out, std::uint32_t number, int bytes) {
                for (int i = 0; i < bytes; ++i) out.push_back(static_cast<char>((number >> (8 * i)) & 0xFF));
            };
            const std::string data(4800 * 4, '\0');
            std::string out = "RIFF";
            append(out, static_cast<std::uint32_t>(36 + data.size()), 4);
            out += "WAVEfmt ";
            append(out, 16, 4);
            append(out, 1, 2);
            append(out, 2, 2);
            append(out, 48000, 4);
            append(out, 48000 * 4, 4);
            append(out, 4, 2);
            append(out, 16, 2);
            out += "data";
            append(out, static_cast<std::uint32_t>(data.size()), 4);
            return write("sound.wav", out + data);
        }

        std::string run(const std::string &code) const
        {
            lua_State *vm = Luau::Instance().getState();
            lua_State *thread = lua_newthread(vm);
            luaL_sandboxthread(thread);
            flecs::entity handle = emitter->getHandle();
            atmo::luau::LuaBindings<Emitter>::Push(thread, Emitter(handle));
            lua_setglobal(thread, "this");

            std::size_t size = 0;
            char *bytecode = Luau::Compile(code, &size, "test");
            REQUIRE(bytecode != nullptr);
            const bool loaded = Luau::LoadBytecodeCoroutine(thread, "test", bytecode, size);
            std::free(bytecode);
            REQUIRE(loaded);

            std::string error;
            if (lua_pcall(thread, 0, 0, 0) != LUA_OK) {
                const char *message = lua_tostring(thread, -1);
                error = message ? message : "unknown error";
            }
            lua_pop(vm, 1);
            return error;
        }
    };
} // namespace

TEST_CASE("scripts read and write the emitter settings", "[audio][luau]")
{
    ScriptWorld test;
    REQUIRE(test.run(R"(
        assert(this:getVolume() == 1 and this:getPitch() == 1 and this:getPan() == 0)
        assert(this:getLoop() == false and this:getAutoplay() == false and this:getAudioPath() == "")
        assert(this:getBus() == "master")
        this:setVolume(0.5)
        this:setPitch(2)
        this:setPan(-0.25)
        this:setLoop(true)
        this:setAutoplay(true)
        this:setBus("music")
        assert(this:getVolume() == 0.5 and this:getPitch() == 2 and this:getPan() == -0.25)
        assert(this:getLoop() == true and this:getAutoplay() == true)
        assert(this:getBus() == "music")
    )")
                .empty());

    REQUIRE(test.emitter->getVolume() == 0.5f);
    REQUIRE(test.emitter->getPitch() == 2.0f);
    REQUIRE(test.emitter->getPan() == -0.25f);
    REQUIRE(test.emitter->getLoop());
    REQUIRE(test.emitter->getAutoplay());
    REQUIRE(test.emitter->getBus() == atmo::server::audio::Bus::Music);
}

TEST_CASE("scripts play, pause and stop a sound", "[audio][luau]")
{
    ScriptWorld test;
    REQUIRE(test.run("this:setAudioPath('" + test.wav() + R"(')
        assert(string.find(this:getAudioPath(), "sound.wav") ~= nil)
        assert(this:isPlaying() == false)
        this:play()
        assert(this:isPlaying() == true)
    )")
                .empty());
    REQUIRE(test.emitter->isPlaying());

    const auto advance = [] {
        std::vector<float> out(4800 * 2);
        atmo::server::audio::AudioServer::Instance().mix(out.data(), 4800);
        atmo::server::audio::AudioServer::Instance().mix(out.data(), 1);
    };
    REQUIRE(test.run("this:setPaused(true)").empty());
    advance();
    REQUIRE(test.emitter->isPlaying());
    REQUIRE(test.run("this:setPaused(false)").empty());
    advance();
    REQUIRE_FALSE(test.emitter->isPlaying());

    REQUIRE(test.run("this:play() this:stop() assert(this:isPlaying() == false)").empty());
    REQUIRE(test.run("this:setAudioPath() assert(this:getAudioPath() == '')").empty());
}

TEST_CASE("scripts keep the Entity methods on an emitter", "[audio][luau]")
{
    ScriptWorld test;
    REQUIRE(test.run("assert(this:isAlive() == true) assert(type(this:name()) == 'string')").empty());
}

TEST_CASE("wrong arguments raise a script error", "[audio][luau]")
{
    const std::string code = GENERATE(
        "this:setVolume('loud')",
        "this:setVolume()",
        "this:setPitch(true)",
        "this:setPan({})",
        "this:setLoop(1)",
        "this:setAutoplay('yes')",
        "this:setPaused()",
        "this:setAudioPath({})",
        "this:setBus()",
        "this:setBus('ambience')",
        "this:setBus(1)",
        "this.play()",
        "this.play({})",
        "this.volume = 1",
        "this:doesNotExist()");
    INFO(code);

    ScriptWorld test;
    REQUIRE_FALSE(test.run(code).empty());
    REQUIRE(test.emitter->getVolume() == 1.0f);
}

TEST_CASE("scripts can hold a destroyed emitter", "[audio][luau]")
{
    ScriptWorld test;
    test.emitter->getHandle().destruct();
    REQUIRE(test.run(R"(
        this:play()
        this:stop()
        this:setPaused(true)
        this:setAudioPath("a.wav")
        this:setVolume(0.5) this:setPitch(2) this:setPan(1) this:setLoop(true) this:setAutoplay(true) this:setBus("sfx")
        assert(this:isPlaying() == false and this:isAlive() == false and this:getBus() == "master")
        assert(this:getVolume() == 1 and this:getPitch() == 1 and this:getPan() == 0)
        assert(this:getLoop() == false and this:getAutoplay() == false and this:getAudioPath() == "")
    )")
                .empty());
}

TEST_CASE("a Script component plays its emitter from Create", "[audio][luau]")
{
    ScriptWorld test;
    const std::string script = test.write(
        "player.luau",
        "function Create()\n    this:setAudioPath('" + test.wav() +
            "')\n    this:setLoop(true)\n    this:play()\nend\n"
            "function Update(dt)\n    this:setVolume(0.25)\nend\n");
    test.emitter->setComponent<atmo::core::components::Script>({ .script_path = script });

    test.world.progress(0.016f);
    REQUIRE(test.emitter->isPlaying());
    test.world.progress(0.016f);
    REQUIRE(test.emitter->getVolume() == 0.25f);
    REQUIRE(test.emitter->getComponent<atmo::core::components::AudioEmitter>().m_loop);
}

TEST_CASE("the registry pushes emitters with their own binding", "[audio][luau]")
{
    ScriptWorld test;
    lua_State *vm = Luau::Instance().getState();
    flecs::entity handle = test.emitter->getHandle();
    REQUIRE(LuauRegistry::Instance().pushEntity(vm, handle));
    lua_getfield(vm, -1, "play");
    REQUIRE(lua_isfunction(vm, -1));
    lua_pop(vm, 2);
}

TEST_CASE("the script of a parent plays its child emitter", "[audio][luau]")
{
    ScriptWorld test;
    auto parent = EntityRegistry::CreateIn(&test.world, "Entity");
    REQUIRE(parent != nullptr);
    test.emitter->rename("Jump");
    test.emitter->setParent(*parent);
    test.emitter->setAudioPath(test.wav());

    const std::string script =
        test.write("parent.luau", "function Create()\n    local sound = this:getChild('Jump')\n    sound:setPitch(2)\n    sound:play()\nend\n");
    parent->setComponent<atmo::core::components::Script>({ .script_path = script });

    test.world.progress(0.016f);
    REQUIRE(test.emitter->isPlaying());
    REQUIRE(test.emitter->getPitch() == 2.0f);
}

TEST_CASE("scripts read and write the bus volumes", "[audio][luau][bus]")
{
    ScopedBuses reset;
    ScriptWorld test;
    REQUIRE(test.run(R"(
        for _, bus in { "master", "music", "sfx" } do
            assert(Audio.getBusVolume(bus) == 1)
        end
        Audio.setBusVolume("music", 0.5)
        Audio.setBusVolume("sfx", -1)
        assert(Audio.getBusVolume("music") == 0.5)
        assert(Audio.getBusVolume("sfx") == 0)
        assert(Audio.getBusVolume("master") == 1)
    )")
                .empty());

    REQUIRE(test.run("assert(Audio.getBusVolume('music') == 0.5)").empty());
    REQUIRE(test.run("Audio.setBusVolume('music', 0/0) assert(Audio.getBusVolume('music') == 0.5)").empty());
}

TEST_CASE("the bus volume set by a script reaches the mix", "[audio][luau][bus]")
{
    ScopedBuses reset;
    auto &server = atmo::server::audio::AudioServer::Instance();
    const auto sample = server.sampleCreate({ 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f });
    const auto voice = server.voiceCreate();
    server.voiceSetSample(voice, sample);
    server.voiceSetBus(voice, atmo::server::audio::Bus::Music);
    server.voicePlay(voice);

    ScriptWorld test;
    const auto mix = [&server] {
        float out[2] = {};
        server.mix(out, 1);
        return out[0];
    };
    REQUIRE(mix() == 0.8f);
    REQUIRE(test.run("Audio.setBusVolume('music', 0.5)").empty());
    REQUIRE(mix() == 0.4f);
    REQUIRE(test.run("Audio.setBusVolume('sfx', 0) Audio.setBusVolume('master', 0.5)").empty());
    REQUIRE(mix() == 0.2f);

    server.voiceFree(voice);
    server.sampleFree(sample);
}

TEST_CASE("wrong bus arguments raise a script error", "[audio][luau][bus]")
{
    const std::string code = GENERATE(
        "Audio.setBusVolume('ambience', 1)",
        "Audio.setBusVolume('Music', 1)",
        "Audio.setBusVolume('music')",
        "Audio.setBusVolume('music', 'loud')",
        "Audio.setBusVolume(0.5)",
        "Audio.setBusVolume()",
        "Audio:setBusVolume('music', 0.5)",
        "Audio.getBusVolume()",
        "Audio.getBusVolume('ambience')",
        "Audio.setBusVolume = nil");
    INFO(code);

    ScopedBuses reset;
    ScriptWorld test;
    REQUIRE_FALSE(test.run(code).empty());
    REQUIRE(test.run("assert(Audio.getBusVolume('music') == 1)").empty());
}

TEST_CASE("a Script component changes the bus volumes from Create", "[audio][luau][bus]")
{
    ScopedBuses reset;
    ScriptWorld test;
    const std::string script = test.write("options.luau", "function Create()\n    Audio.setBusVolume('music', 0.25)\nend\n");
    test.emitter->setComponent<atmo::core::components::Script>({ .script_path = script });

    test.world.progress(0.016f);
    REQUIRE(atmo::server::audio::AudioServer::Instance().busGetVolume(atmo::server::audio::Bus::Music) == 0.25f);
}

TEST_CASE("scripts add, change and remove effects", "[audio][luau][effects]")
{
    ScopedBuses reset;
    ScriptWorld test;
    REQUIRE(test.run(R"(
        assert(Audio.getEffect("music", "low_pass") == nil)
        Audio.setEffect("music", "low_pass", { cutoff = 400 })
        local low_pass = Audio.getEffect("music", "low_pass")
        assert(low_pass.cutoff == 400 and low_pass.resonance > 0)
        assert(Audio.getEffect("sfx", "low_pass") == nil)

        Audio.setEffect("music", "low_pass", { resonance = 2 })
        low_pass = Audio.getEffect("music", "low_pass")
        assert(low_pass.cutoff == 400 and low_pass.resonance == 2)
        Audio.setEffect("music", "low_pass")
        assert(Audio.getEffect("music", "low_pass").cutoff == 400)

        Audio.removeEffect("music", "low_pass")
        Audio.removeEffect("music", "low_pass")
        assert(Audio.getEffect("music", "low_pass") == nil)

        for _, effect in { "high_pass", "low_pass", "eq", "delay", "reverb" } do
            Audio.setEffect("master", effect)
            assert(Audio.getEffect("master", effect) ~= nil)
        end

        Audio.setDucking("music", "sfx", { amount = 0.5, release = 1 })
        Audio.setDucking("music", "sfx")
        Audio.removeDucking("music")
    )")
                .empty());
}

TEST_CASE("wrong effect arguments raise a script error and change nothing", "[audio][luau][effects]")
{
    const std::string code = GENERATE(
        "Audio.setEffect()",
        "Audio.setEffect('music')",
        "Audio.setEffect('music', 'lowpass')",
        "Audio.setEffect('ambience', 'low_pass')",
        "Audio.setEffect('music', 'low_pass', 400)",
        "Audio.setEffect('music', 'low_pass', { cutof = 400 })",
        "Audio.setEffect('music', 'low_pass', { 400 })",
        "Audio.setEffect('music', 'low_pass', { cutoff = 'high' })",
        "Audio.setEffect('music', 'low_pass', { cutoff = '400' })",
        "Audio.setEffect('music', 'low_pass', { cutoff = 400, room_size = 1 })",
        "Audio.getEffect('music')",
        "Audio.getEffect('music', 'limiter')",
        "Audio.removeEffect('music')",
        "Audio.setDucking('music')",
        "Audio.setDucking('music', 'music')",
        "Audio.setDucking('music', 'master')",
        "Audio.setDucking('master', 'sfx')",
        "Audio.setDucking('music', 'sfx', { cutoff = 400 })",
        "Audio.removeDucking()");
    INFO(code);

    ScopedBuses reset;
    ScriptWorld test;
    REQUIRE_FALSE(test.run(code).empty());
    REQUIRE(test.run("assert(Audio.getEffect('music', 'low_pass') == nil)").empty());
}

TEST_CASE("the error of a wrong effect or setting lists the right ones", "[audio][luau][effects]")
{
    ScopedBuses reset;
    ScriptWorld test;
    REQUIRE(test.run("Audio.setEffect('music', 'lowpass')").find("high_pass, low_pass, eq, delay, reverb") != std::string::npos);
    REQUIRE(test.run("Audio.setEffect('music', 'delay', { speed = 1 })").find("time, feedback, mix") != std::string::npos);
}
