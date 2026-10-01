#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "common/math.hpp"

using Catch::Approx;

TEST_CASE("Degrees and radians conversions", "[math]")
{
    REQUIRE(atmo::common::math::DegreesToRadians(0.0f) == Approx(0.0f));
    REQUIRE(atmo::common::math::DegreesToRadians(90.0f) == Approx(atmo::common::math::PI / 2.0f));
    REQUIRE(atmo::common::math::DegreesToRadians(180.0f) == Approx(atmo::common::math::PI));
    REQUIRE(atmo::common::math::DegreesToRadians(360.0f) == Approx(2.0f * atmo::common::math::PI));
    REQUIRE(atmo::common::math::DegreesToRadians(-90.0f) == Approx(-atmo::common::math::PI / 2.0f));

    REQUIRE(atmo::common::math::RadiansToDegrees(0.0f) == Approx(0.0f));
    REQUIRE(atmo::common::math::RadiansToDegrees(atmo::common::math::PI) == Approx(180.0f));
    REQUIRE(atmo::common::math::RadiansToDegrees(-atmo::common::math::PI / 2.0f) == Approx(-90.0f));
    REQUIRE(atmo::common::math::RadiansToDegrees(atmo::common::math::DegreesToRadians(123.5f)) == Approx(123.5f));
}

TEST_CASE("Clamp values", "[math]")
{
    REQUIRE(atmo::common::math::Clamp(1, 2, 5) == 2);
    REQUIRE(atmo::common::math::Clamp(6, 2, 5) == 5);
    REQUIRE(atmo::common::math::Clamp(3, 2, 5) == 3);
    REQUIRE(atmo::common::math::Clamp(2, 2, 5) == 2);
    REQUIRE(atmo::common::math::Clamp(5, 2, 5) == 5);
    REQUIRE(atmo::common::math::Clamp(-1.5f, -1.0f, 2.0f) == Approx(-1.0f));
    REQUIRE(atmo::common::math::Clamp(3.5f, -1.0f, 2.0f) == Approx(2.0f));
    REQUIRE(atmo::common::math::Clamp(0.5f, -1.0f, 2.0f) == Approx(0.5f));
}

TEST_CASE("Lerp values", "[math]")
{
    REQUIRE(atmo::common::math::Lerp(10.0f, 20.0f, 0.0f) == Approx(10.0f));
    REQUIRE(atmo::common::math::Lerp(10.0f, 20.0f, 1.0f) == Approx(20.0f));
    REQUIRE(atmo::common::math::Lerp(10.0f, 20.0f, 0.5f) == Approx(15.0f));
    REQUIRE(atmo::common::math::Lerp(-10.0f, -2.0f, 0.5f) == Approx(-6.0f));
}

TEST_CASE("Round values", "[math]")
{
    REQUIRE(atmo::common::math::Round(1.4f) == Approx(1.0f));
    REQUIRE(atmo::common::math::Round(1.5f) == Approx(2.0f));
    REQUIRE(atmo::common::math::Round(-1.5f) == Approx(-2.0f));
    REQUIRE(atmo::common::math::Round(1.4) == Approx(1.0));
    REQUIRE(atmo::common::math::Round(2.5) == Approx(3.0));
    REQUIRE(atmo::common::math::Round(-2.5) == Approx(-3.0));
}
