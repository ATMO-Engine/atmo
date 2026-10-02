#include <catch2/catch_test_macros.hpp>

#include "common/utils.hpp"

TEST_CASE("GlobMatch", "[utils]")
{
    REQUIRE(atmo::common::Utils::GlobMatch("readme.txt", "readme.txt"));
    REQUIRE(atmo::common::Utils::GlobMatch("readme.???", "readme.txt"));
    REQUIRE(atmo::common::Utils::GlobMatch("*.txt", "readme.txt"));
    REQUIRE(atmo::common::Utils::GlobMatch("src/**/main.*", "src/core/main.cpp"));
    REQUIRE(atmo::common::Utils::GlobMatch("", ""));
    REQUIRE(atmo::common::Utils::GlobMatch("*", ""));

    REQUIRE_FALSE(atmo::common::Utils::GlobMatch("", "readme.txt"));
    REQUIRE_FALSE(atmo::common::Utils::GlobMatch("readme.???", "readme.md"));
    REQUIRE_FALSE(atmo::common::Utils::GlobMatch("*.txt", "readme.cpp"));
    REQUIRE_FALSE(atmo::common::Utils::GlobMatch("src/**/main.*", "src/core/app.cpp"));
}
