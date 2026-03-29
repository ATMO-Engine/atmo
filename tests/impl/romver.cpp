#include <catch2/catch_test_macros.hpp>

#include "impl/romver.hpp"

TEST_CASE("Simple version Parse method", "[romver]")
{
    std::string version = "1.0.4";

    atmo::impl::Romver result = atmo::impl::Romver::Parse(version);

    REQUIRE(version == result.to_string());
}

TEST_CASE("Simple version constructor", "[romver]")
{
    std::string version = "1.0.4";

    atmo::impl::Romver result(version);

    REQUIRE(version == result.to_string());
}

TEST_CASE("Version + pre", "[romver]")
{
    std::string version = "1.0.0-alpha";

    atmo::impl::Romver result(version);

    REQUIRE(version == result.to_string());
}

TEST_CASE("Version + build", "[romver]")
{
    std::string version = "1.0.0+build";

    atmo::impl::Romver result(version);

    REQUIRE(version == result.to_string());
}

TEST_CASE("Version + pre + build", "[romver]")
{
    std::string version = "1.0.0-alpha+build.1";

    atmo::impl::Romver result(version);

    REQUIRE(version == result.to_string());
}

TEST_CASE("Invalid string 1", "[romver]")
{
    std::string version = "1.a.4";

    atmo::impl::Romver result(version);

    REQUIRE(result.to_string() == "0.0.1");
}

TEST_CASE("Invalid string 2", "[romver]")
{
    std::string version = "1a0.4";

    atmo::impl::Romver result(version);

    REQUIRE(result.to_string() == "0.0.1");
}
