#include <array>
#include <cstdint>
#include <cstring>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "server/rendering/shader/shader_compiler.hpp"

using namespace atmo::server::rendering;
using Catch::Matchers::ContainsSubstring;

namespace
{
    constexpr std::array material_targets{ ShaderTarget::SpirV, ShaderTarget::Msl };

    ShaderCompiler &Compiler()
    {
        static ShaderCompiler compiler;
        return compiler;
    }

    std::string CodeText(const ShaderCode &code)
    {
        return { reinterpret_cast<const char *>(code.code.data()), code.code.size() };
    }

    constexpr const char *wobble_source = R"(
import atmo;

public struct Wobble : atmo::ICanvasMaterial
{
    [Range(0, 0.1)] float strength;
    [Color] float4 tint;
    float3 offset;
    uint mode;
    atmo::Texture noise;
    float2 speed;

    float4 fragment(atmo::CanvasFragment input)
    {
        float wave = sin(input.uv.y * 30.0 + atmo::time() * speed.x) * strength;
        float grain = noise.sample(input.uv * 4.0).r;
        return input.sample(input.uv + float2(wave, 0.0)) * input.color * tint * grain + float4(offset, float(mode)) * input.custom;
    }
};
)";
} // namespace

TEST_CASE("a canvas material is compiled for every target and its parameters are reflected", "[rendering][shader][material]")
{
    auto compiled = Compiler().compileMaterial({ .name = "wobble", .path = "shaders/wobble.slang", .code = wobble_source }, material_targets);
    REQUIRE(compiled.has_value());

    REQUIRE(compiled->type_name == "Wobble");
    const ShaderDesc &desc = compiled->desc;
    REQUIRE(desc.kind == ShaderKind::Canvas);
    REQUIRE(desc.entry_point == "materialFragmentMain");
    REQUIRE(desc.code.size() == 2);

    const ShaderParamLayout &layout = desc.layout;
    REQUIRE(layout.params.size() == 6);
    const std::array<std::pair<const char *, ShaderParamType>, 6> expected{ {
        { "strength", ShaderParamType::Float },
        { "tint", ShaderParamType::Color },
        { "offset", ShaderParamType::Float3 },
        { "mode", ShaderParamType::Uint },
        { "noise", ShaderParamType::Texture },
        { "speed", ShaderParamType::Float2 },
    } };
    std::uint32_t offset = 0;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        INFO(expected[i].first);
        REQUIRE(layout.params[i].name == expected[i].first);
        REQUIRE(layout.params[i].type == expected[i].second);
        REQUIRE(layout.params[i].offset == offset);
        offset += ShaderParamSize(expected[i].second);
    }
    REQUIRE(layout.size == offset);
    REQUIRE(layout.size == 52);

    REQUIRE(layout.find("strength")->range.has_value());
    REQUIRE(layout.find("strength")->range->at(0) == 0.0f);
    REQUIRE(layout.find("strength")->range->at(1) == 0.1f);
    REQUIRE_FALSE(layout.find("tint")->range.has_value());

    const ShaderCode *spirv = desc.codeFor(ShaderTarget::SpirV);
    REQUIRE(spirv);
    std::uint32_t magic = 0;
    std::memcpy(&magic, spirv->code.data(), sizeof(magic));
    REQUIRE(magic == 0x07230203);

    const ShaderCode *msl = desc.codeFor(ShaderTarget::Msl);
    REQUIRE(msl);
    const std::string text = CodeText(*msl);
    REQUIRE_THAT(text, ContainsSubstring("materialFragmentMain"));
    REQUIRE_THAT(text, ContainsSubstring("[[buffer(30)]]"));
    REQUIRE_THAT(text, ContainsSubstring("[[buffer(0)]]"));
}

TEST_CASE("a viewport material is recognised as a post-processing shader", "[rendering][shader][material]")
{
    constexpr const char *source = R"(
import atmo;

namespace effects
{
    public struct Grayscale : atmo::IViewportMaterial
    {
        [Range(0, 1)] float amount;

        float4 fragment(atmo::ViewportFragment input)
        {
            float4 color = input.sample(input.uv);
            float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
            return float4(lerp(color.rgb, float3(gray, gray, gray), amount), color.a);
        }
    };
}
)";
    auto compiled = Compiler().compileMaterial({ .name = "grayscale", .code = source }, material_targets);
    REQUIRE(compiled.has_value());
    REQUIRE(compiled->type_name == "effects::Grayscale");
    REQUIRE(compiled->desc.kind == ShaderKind::Viewport);
    REQUIRE(compiled->desc.layout.size == 4);
}

TEST_CASE("material sources with no material, two materials or unsupported parameters are rejected", "[rendering][shader][material]")
{
    auto none = Compiler().compileMaterial({ .name = "none", .code = "import atmo;\npublic struct Plain { float x; };\n" }, material_targets);
    REQUIRE_FALSE(none.has_value());
    REQUIRE_THAT(none.error(), ContainsSubstring("no struct that implements"));

    constexpr const char *two = R"(
import atmo;
public struct A : atmo::ICanvasMaterial { float4 fragment(atmo::CanvasFragment input) { return input.color; } };
public struct B : atmo::ICanvasMaterial { float4 fragment(atmo::CanvasFragment input) { return input.color; } };
)";
    auto both = Compiler().compileMaterial({ .name = "two", .code = two }, material_targets);
    REQUIRE_FALSE(both.has_value());
    REQUIRE_THAT(both.error(), ContainsSubstring("exactly one material"));
    REQUIRE_THAT(both.error(), ContainsSubstring("A, B"));

    constexpr const char *matrix = R"(
import atmo;
public struct M : atmo::ICanvasMaterial { float4x4 transform; float4 fragment(atmo::CanvasFragment input) { return mul(transform, input.color); } };
)";
    auto unsupported = Compiler().compileMaterial({ .name = "matrix", .code = matrix }, material_targets);
    REQUIRE_FALSE(unsupported.has_value());
    REQUIRE_THAT(unsupported.error(), ContainsSubstring("parameter 'transform'"));

    constexpr const char *bad_color = R"(
import atmo;
public struct M : atmo::ICanvasMaterial { [Color] float3 tint; float4 fragment(atmo::CanvasFragment input) { return input.color * float4(tint, 1.0); } };
)";
    auto color = Compiler().compileMaterial({ .name = "color", .code = bad_color }, material_targets);
    REQUIRE_FALSE(color.has_value());
    REQUIRE_THAT(color.error(), ContainsSubstring("[Color]"));

    constexpr const char *flag = R"(
import atmo;
public struct M : atmo::ICanvasMaterial { bool on; float4 fragment(atmo::CanvasFragment input) { return on ? input.color : 0.0; } };
)";
    auto boolean = Compiler().compileMaterial({ .name = "flag", .code = flag }, material_targets);
    REQUIRE_FALSE(boolean.has_value());
    REQUIRE_THAT(boolean.error(), ContainsSubstring("use a uint"));

    auto syntax = Compiler().compileMaterial({ .name = "syntax", .path = "shaders/broken.slang", .code = "import atmo;\npublic struct {" }, material_targets);
    REQUIRE_FALSE(syntax.has_value());
    REQUIRE_THAT(syntax.error(), ContainsSubstring("broken.slang"));
}
