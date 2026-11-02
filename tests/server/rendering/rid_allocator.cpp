#include <type_traits>
#include <unordered_set>

#include <catch2/catch_test_macros.hpp>

#include "server/rendering/rid_allocator.hpp"

using namespace atmo::server::rendering;

TEST_CASE("default constructed RIDs are null", "[rendering][rid]")
{
    WindowRid rid;
    REQUIRE_FALSE(rid.isValid());
    REQUIRE(rid.raw() == 0);
}

TEST_CASE("RIDs of different kinds are distinct types", "[rendering][rid]")
{
    STATIC_REQUIRE_FALSE(std::is_same_v<TextureRid, CanvasItemRid>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<TextureRid, CanvasItemRid>);
    STATIC_REQUIRE(sizeof(TextureRid) == sizeof(std::uint32_t));
}

TEST_CASE("allocate returns valid, unique, alive RIDs", "[rendering][rid]")
{
    RidAllocator<RidKind::Texture> allocator;
    std::unordered_set<TextureRid> seen;

    for (int i = 0; i < 100; ++i) {
        TextureRid rid = allocator.allocate();
        REQUIRE(rid.isValid());
        REQUIRE(allocator.isAlive(rid));
        REQUIRE(seen.insert(rid).second);
    }

    REQUIRE(allocator.aliveCount() == 100);
}

TEST_CASE("release frees a RID exactly once", "[rendering][rid]")
{
    RidAllocator<RidKind::Texture> allocator;
    TextureRid rid = allocator.allocate();

    REQUIRE(allocator.release(rid));
    REQUIRE_FALSE(allocator.isAlive(rid));
    REQUIRE_FALSE(allocator.release(rid));
    REQUIRE(allocator.aliveCount() == 0);
}

TEST_CASE("a released slot is reused with a new generation", "[rendering][rid]")
{
    RidAllocator<RidKind::CanvasItem> allocator;
    CanvasItemRid first = allocator.allocate();
    REQUIRE(allocator.release(first));

    CanvasItemRid second = allocator.allocate();

    REQUIRE(second.slot() == first.slot());
    REQUIRE(second.generation() != first.generation());
    REQUIRE(second != first);
    REQUIRE(allocator.isAlive(second));
    REQUIRE_FALSE(allocator.isAlive(first));
    REQUIRE_FALSE(allocator.release(first));
    REQUIRE(allocator.isAlive(second));
    REQUIRE(allocator.capacity() == 1);
}

TEST_CASE("generations wrap without ever producing the null RID", "[rendering][rid]")
{
    RidAllocator<RidKind::Texture> allocator;
    TextureRid first = allocator.allocate();

    TextureRid rid = first;
    for (int i = 0; i < 600; ++i) {
        REQUIRE(allocator.release(rid));
        rid = allocator.allocate();
        REQUIRE(rid.isValid());
        REQUIRE(rid.slot() == first.slot());
        REQUIRE(rid.generation() != 0);
    }
}

TEST_CASE("RIDs from another allocator are not alive", "[rendering][rid]")
{
    RidAllocator<RidKind::Texture> a;
    RidAllocator<RidKind::Texture> b;

    TextureRid rid = a.allocate();
    REQUIRE_FALSE(b.isAlive(rid));
    REQUIRE_FALSE(b.release(rid));
}
