#include "core/args/arg_manager.hpp"
#include <catch2/catch_test_macros.hpp>

#include "atmo.hpp"
#include "catch2/catch_approx.hpp"
#include "spdlog/spdlog.h"

TEST_CASE("single argument with --", "[args]")
{
    atmo::core::args::ArgManager::Parse({ "--testarg" });

    REQUIRE(atmo::core::args::ArgManager::HasArg("testarg") == true);
    REQUIRE(std::get<bool>(atmo::core::args::ArgManager::GetArgValue("testarg")) == true);
}

TEST_CASE("single argument with -", "[args]")
{
    atmo::core::args::ArgManager::Parse({ "-a" });

    REQUIRE(atmo::core::args::ArgManager::HasArg("a") == true);
    REQUIRE(std::get<bool>(atmo::core::args::ArgManager::GetArgValue("a")) == true);
}

TEST_CASE("positional argument", "[args]")
{
    atmo::core::args::ArgManager::Parse({ "positionalarg" });

    REQUIRE(atmo::core::args::ArgManager::HasArg("positionalarg") == true);
    REQUIRE(std::get<std::monostate>(atmo::core::args::ArgManager::GetArgValue("positionalarg")) == std::monostate());
}

TEST_CASE("arguments with values and '=' sign", "[args]")
{
    atmo::core::args::ArgManager::Parse({ "--intarg=42", "--floatarg=3.14", "--stringarg=hello" });

    REQUIRE(atmo::core::args::ArgManager::HasArg("intarg") == true);
    REQUIRE(std::get<int>(atmo::core::args::ArgManager::GetArgValue("intarg")) == 42);

    REQUIRE(atmo::core::args::ArgManager::HasArg("floatarg") == true);
    REQUIRE(std::get<float>(atmo::core::args::ArgManager::GetArgValue("floatarg")) == 3.14f);

    REQUIRE(atmo::core::args::ArgManager::HasArg("stringarg") == true);
    REQUIRE(std::get<std::string>(atmo::core::args::ArgManager::GetArgValue("stringarg")) == "hello");
}

TEST_CASE("arguments with values and spaces", "[args]")
{
    atmo::core::args::ArgManager::Parse({ "--intarg", "100", "--floatarg", "2.71", "--stringarg", "world" });

    REQUIRE(atmo::core::args::ArgManager::HasArg("intarg") == true);
    REQUIRE(std::get<int>(atmo::core::args::ArgManager::GetArgValue("intarg")) == 100);

    REQUIRE(atmo::core::args::ArgManager::HasArg("floatarg") == true);
    REQUIRE(std::get<float>(atmo::core::args::ArgManager::GetArgValue("floatarg")) == Catch::Approx(2.71f));

    REQUIRE(atmo::core::args::ArgManager::HasArg("stringarg") == true);
    REQUIRE(std::get<std::string>(atmo::core::args::ArgManager::GetArgValue("stringarg")) == "world");
}

TEST_CASE("argument not provided", "[args]")
{
    atmo::core::args::ArgManager::Parse({});

    REQUIRE(atmo::core::args::ArgManager::HasArg("missingarg") == false);
}
