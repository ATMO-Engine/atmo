#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "server/rendering/shader/builtin_shaders.hpp"
#include "server/rendering/shader/shader_compiler.hpp"

using namespace atmo::server::rendering;

namespace
{
    constexpr unsigned char test_pattern_source[] = {
#include "shaders/test_pattern.slang.h"
    };

    constexpr std::uint32_t spirv_magic = 0x07230203;

    std::uint32_t FirstWord(const std::vector<std::byte> &code)
    {
        std::uint32_t word = 0;
        std::memcpy(&word, code.data(), sizeof(word));
        return word;
    }

    ShaderCompiler &Compiler()
    {
        static ShaderCompiler compiler;
        return compiler;
    }
} // namespace

TEST_CASE("the shader compiler builds the test pattern for SPIR-V and reflects it", "[rendering][shader]")
{
    const ShaderSource source{
        .name = "test_pattern",
        .code = std::string(reinterpret_cast<const char *>(test_pattern_source), sizeof(test_pattern_source)),
    };

    auto compiled = Compiler().compile(source, ShaderTarget::SpirV);
    REQUIRE(compiled.has_value());

    REQUIRE(compiled->target == ShaderTarget::SpirV);
    REQUIRE(compiled->code.size() % 4 == 0);
    REQUIRE(FirstWord(compiled->code) == spirv_magic);

    const ShaderLayout &layout = compiled->layout;
    REQUIRE(layout.entry_points.size() == 2);
    auto has_entry = [&layout](std::string_view name, ShaderStage stage) {
        return std::ranges::any_of(layout.entry_points, [&](const ShaderEntryPoint &entry) { return entry.name == name && entry.stage == stage; });
    };
    REQUIRE(has_entry("vertexMain", ShaderStage::Vertex));
    REQUIRE(has_entry("fragmentMain", ShaderStage::Fragment));

    REQUIRE(layout.push_constant_size == 48);

    auto binding = [&layout](ShaderBindingKind kind) {
        return std::ranges::find_if(layout.bindings, [kind](const ShaderBinding &b) { return b.kind == kind; });
    };
    auto textures = binding(ShaderBindingKind::Texture);
    REQUIRE(textures != layout.bindings.end());
    REQUIRE(textures->set == 0);
    REQUIRE(textures->binding == 0);
    REQUIRE(textures->count == 0);

    auto samplers = binding(ShaderBindingKind::Sampler);
    REQUIRE(samplers != layout.bindings.end());
    REQUIRE(samplers->set == 0);
    REQUIRE(samplers->binding == 1);
    REQUIRE(samplers->count == 0);
}

TEST_CASE("shader compile errors come back as diagnostics", "[rendering][shader]")
{
    const ShaderSource source{
        .name = "broken",
        .code = "[shader(\"fragment\")] float4 fragmentMain() : SV_Target { return undefined_symbol; }",
    };

    auto compiled = Compiler().compile(source, ShaderTarget::SpirV);
    REQUIRE_FALSE(compiled.has_value());
    REQUIRE(compiled.error().find("undefined_symbol") != std::string::npos);
}

TEST_CASE("shaders without entry points are rejected", "[rendering][shader]")
{
    auto compiled = Compiler().compile({ .name = "library", .code = "float twice(float x) { return x * 2.0; }" }, ShaderTarget::SpirV);
    REQUIRE_FALSE(compiled.has_value());
    REQUIRE(compiled.error() == "the shader defines no entry points");
}

TEST_CASE("unsupported shader targets are refused", "[rendering][shader]")
{
    REQUIRE_FALSE(ShaderCompiler::Supports(ShaderTarget::None));
    REQUIRE(ShaderCompiler::Supports(ShaderTarget::SpirV));

    auto compiled = Compiler().compile({ .name = "any", .code = "" }, ShaderTarget::None);
    REQUIRE_FALSE(compiled.has_value());
}

#if defined(ATMO_RENDERING_VULKAN)
TEST_CASE("the built-in SPIR-V test pattern is embedded", "[rendering][shader]")
{
    std::optional<ShaderBlob> blob = BuiltinShaderBlob(BuiltinShader::TestPattern, ShaderTarget::SpirV);
    REQUIRE(blob.has_value());
    REQUIRE(blob->code.size() % 4 == 0);
    REQUIRE(FirstWord({ blob->code.begin(), blob->code.end() }) == spirv_magic);
    REQUIRE(reinterpret_cast<std::uintptr_t>(blob->code.data()) % 4 == 0);
}
#endif

TEST_CASE("the Null target gets an empty built-in blob", "[rendering][shader]")
{
    std::optional<ShaderBlob> blob = BuiltinShaderBlob(BuiltinShader::TestPattern, ShaderTarget::None);
    REQUIRE(blob.has_value());
    REQUIRE(blob->code.empty());
    REQUIRE_FALSE(BuiltinShaderBlob(BuiltinShader::TestPattern, ShaderTarget::Dxil).has_value());
}
