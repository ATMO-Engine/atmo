#include <catch2/catch_test_macros.hpp>

#include "core/engine.hpp"

TEST_CASE("Engine initializes correctly", "[engine]")
{
    atmo::core::Engine engine;

    REQUIRE(engine.get_ecs().c_ptr() != nullptr);
    REQUIRE(engine.get_prefabs().size() > 0);
}
