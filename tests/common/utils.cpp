#include <catch2/catch_test_macros.hpp>

#include <string>

#include "common/utils.hpp"

using atmo::common::Utils;

TEST_CASE("PopBackUtf8 leaves an empty string untouched", "[utils][utf8]")
{
    std::string str;

    Utils::PopBackUtf8(str);

    REQUIRE(str.empty());
}

TEST_CASE("PopBackUtf8 removes a single ASCII character", "[utils][utf8]")
{
    std::string str = "Alpha";

    Utils::PopBackUtf8(str);

    REQUIRE(str == "Alph");
}

TEST_CASE("PopBackUtf8 removes a two-byte character whole", "[utils][utf8]")
{
    std::string str = "caf\xc3\xa9";

    Utils::PopBackUtf8(str);

    REQUIRE(str == "caf");
}

TEST_CASE("PopBackUtf8 removes a three-byte character whole", "[utils][utf8]")
{
    std::string str = "\xe4\xb8\xad"; // U+4E2D

    Utils::PopBackUtf8(str);

    REQUIRE(str.empty());
}

TEST_CASE("PopBackUtf8 removes a four-byte character whole", "[utils][utf8]")
{
    std::string str = "hi\xf0\x9f\x98\x80"; // U+1F600

    Utils::PopBackUtf8(str);

    REQUIRE(str == "hi");
}

TEST_CASE("PopBackUtf8 keeps earlier multi-byte characters", "[utils][utf8]")
{
    std::string str = "\xc3\xa9\xc3\xa8";

    Utils::PopBackUtf8(str);

    REQUIRE(str == "\xc3\xa9");
}

TEST_CASE("PopBackUtf8 drops a whole code point from a mixed string", "[utils][utf8]")
{
    std::string str = std::string("a\xc3\xa9\xe4\xb8\xad") + "b";

    Utils::PopBackUtf8(str);

    REQUIRE(str == "a\xc3\xa9\xe4\xb8\xad");
}

TEST_CASE("PopBackUtf8 drops the lead byte of a truncated sequence", "[utils][utf8]")
{
    // Malformed: "\xa9" is a continuation byte, so the last code point starts at 'b'. Removing
    // it drops "b\xa9" and leaves "a" rather than leaving a dangling continuation byte.
    std::string str = "ab\xa9";

    Utils::PopBackUtf8(str);

    REQUIRE(str == "a");
}

TEST_CASE("PopBackUtf8 bounds the walk on a string of only continuation bytes", "[utils][utf8]")
{
    // Six continuation bytes with no lead byte anywhere. The count saturates at three, and a
    // byte is still in front of them, so 3 + 1 = 4 bytes go and 6 - 4 = 2 remain.
    std::string str = "\x80\x80\x80\x80\x80\x80";

    Utils::PopBackUtf8(str);

    REQUIRE(str == "\x80\x80");
}

TEST_CASE("PopBackUtf8 empties a string of only continuation bytes", "[utils][utf8]")
{
    // Every byte is a continuation byte and there is no lead byte to remove, so the counted
    // bytes are all that can go. This must not underflow the size computation.
    std::string str = "\x80\x80\x80";

    Utils::PopBackUtf8(str);

    REQUIRE(str.empty());
}

TEST_CASE("PopBackUtf8 empties a single continuation byte", "[utils][utf8]")
{
    std::string str = "\x80";

    Utils::PopBackUtf8(str);

    REQUIRE(str.empty());
}

TEST_CASE("PopBackUtf8 empties a two-byte continuation run", "[utils][utf8]")
{
    std::string str = "\x80\x80";

    Utils::PopBackUtf8(str);

    REQUIRE(str.empty());
}

TEST_CASE("PopBackUtf8 removes the lead byte and three continuation bytes", "[utils][utf8]")
{
    // 'a' followed by five continuation bytes: the walk stops at the three-byte cap, so the code
    // point that gets removed is 'a' plus three continuation bytes.
    std::string str = "a\x80\x80\x80\x80\x80";

    Utils::PopBackUtf8(str);

    REQUIRE(str == "a\x80");
}
