#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "server/audio/rid_allocator.hpp"

using namespace atmo::server::audio;

TEST_CASE("audio RIDs are null by default and distinct per kind", "[audio][rid]")
{
    REQUIRE_FALSE(SampleRid{}.isValid());
    REQUIRE_FALSE(VoiceRid{}.isValid());
    STATIC_REQUIRE_FALSE(std::is_same_v<SampleRid, VoiceRid>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<SampleRid, VoiceRid>);
}

TEST_CASE("audio RIDs are released exactly once", "[audio][rid]")
{
    RidAllocator<RidKind::Voice> allocator;
    const VoiceRid rid = allocator.allocate();
    REQUIRE(rid.isValid());
    REQUIRE(allocator.isAlive(rid));

    REQUIRE(allocator.release(rid));
    REQUIRE_FALSE(allocator.isAlive(rid));
    REQUIRE_FALSE(allocator.release(rid));
    REQUIRE(allocator.aliveCount() == 0);
}

TEST_CASE("a released audio RID stays dead when its slot is reused", "[audio][rid]")
{
    RidAllocator<RidKind::Sample> allocator;
    const SampleRid first = allocator.allocate();
    REQUIRE(allocator.release(first));

    const SampleRid second = allocator.allocate();
    REQUIRE(second.slot() == first.slot());
    REQUIRE(second != first);
    REQUIRE(allocator.isAlive(second));
    REQUIRE_FALSE(allocator.isAlive(first));
    REQUIRE_FALSE(allocator.release(first));
    REQUIRE(allocator.isAlive(second));
}
