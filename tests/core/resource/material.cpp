#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <variant>

#include <catch2/catch_test_macros.hpp>

#include "core/resource/material.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/shader.hpp"
#include "luau/bindings/resource/bindings_material.hpp"
#include "luau/luau.hpp"
#include "project/project_manager.hpp"
#include "server/rendering/rendering_server.hpp"

using namespace atmo::core::resource;
using namespace atmo::server::rendering;

namespace
{
    constexpr const char *glow_v1 = R"(
import atmo;
import glow_common;

public struct Glow : atmo::ICanvasMaterial
{
    [Range(0, 2)] float strength;
    [Color] float4 tint;
    atmo::Texture mask;

    float4 fragment(atmo::CanvasFragment input)
    {
        return boost(input.sample(input.uv) * tint, strength) * mask.sample(input.uv).r;
    }
};
)";

    constexpr const char *glow_v2 = R"(
import atmo;

public struct Glow : atmo::ICanvasMaterial
{
    [Color] float4 tint;
    float pulse;
    [Range(0, 2)] float strength;

    float4 fragment(atmo::CanvasFragment input)
    {
        return input.color * tint * strength * pulse;
    }
};
)";

    constexpr const char *glow_common = R"(
public float4 boost(float4 color, float amount)
{
    return float4(color.rgb * (1.0 + amount), color.a);
}
)";

    struct Project {
        Project() : root(std::filesystem::temp_directory_path() / "atmo-material-test")
        {
            std::filesystem::remove_all(root);
            std::filesystem::create_directories(root / "shaders");
            write("shaders/glow.slang", glow_v1);
            write("shaders/glow_common.slang", glow_common);
            REQUIRE(RenderingServer::Instance().init({ .platform = PlatformKind::Null, .render_thread = false }));
        }

        ~Project()
        {
            RenderingServer::Instance().shutdown();
            std::error_code error;
            std::filesystem::remove_all(root, error);
        }

        void write(const std::string &relative, const std::string &content) const
        {
            std::ofstream out(root / relative, std::ios::binary | std::ios::trunc);
            out << content;
        }

        [[nodiscard]] std::string path(const std::string &relative) const
        {
            return (root / relative).generic_string();
        }

        std::filesystem::path root;
    };

    std::array<float, 4> Float4(const ShaderValue &value)
    {
        return std::get<std::array<float, 4>>(value);
    }
} // namespace

TEST_CASE("a compiled shader survives serialization", "[resource][material]")
{
    Project project;
    auto desc = Shader::CompileFile(project.path("shaders/glow.slang"));
    REQUIRE(desc.has_value());

    const std::vector<std::byte> bytes = Shader::Serialize(*desc);
    REQUIRE_FALSE(bytes.empty());
    std::optional<ShaderDesc> restored = Shader::Deserialize({ reinterpret_cast<const char *>(bytes.data()), bytes.size() });
    REQUIRE(restored.has_value());
    REQUIRE(restored->name == desc->name);
    REQUIRE(restored->kind == ShaderKind::Canvas);
    REQUIRE(restored->entry_point == desc->entry_point);
    REQUIRE(restored->layout.size == desc->layout.size);
    REQUIRE(restored->layout.params.size() == 3);
    REQUIRE(restored->layout.params[0].name == "strength");
    REQUIRE(restored->layout.params[0].range == desc->layout.params[0].range);
    REQUIRE(restored->layout.params[2].type == ShaderParamType::Texture);
    REQUIRE(restored->code.size() == desc->code.size());
    REQUIRE(restored->code[0].code == desc->code[0].code);

    REQUIRE_FALSE(Shader::Deserialize("not a shader").has_value());
}

TEST_CASE("a material file loads its shader, applies its values and saves them back", "[resource][material]")
{
    Project project;
    project.write(
        "glow.material",
        R"({ "shader": ")" + project.path("shaders/glow.slang") +
            R"(", "params": { "strength": 1.5, "tint": [1, 0.5, 0.25, 1], "mask": { "texture": "", "filter": "nearest", "repeat": true }, "gone": 3 } })");

    std::shared_ptr<Material> material = ResourceManager::GetInstance().getResource<Material>(project.path("glow.material"));
    REQUIRE(material);
    REQUIRE(material->shader());
    REQUIRE(material->kind() == ShaderKind::Canvas);

    RenderingServer &server = RenderingServer::Instance();
    const MaterialRid rid = material->rid();
    REQUIRE(rid.isValid());
    REQUIRE(std::get<float>(*server.materialGetParam(rid, "strength")) == 1.5f);
    REQUIRE(Float4(*server.materialGetParam(rid, "tint")) == std::array{ 1.0f, 0.5f, 0.25f, 1.0f });
    REQUIRE(std::get<MaterialTexture>(*server.materialGetParam(rid, "mask")).filter == TextureFilter::Nearest);
    REQUIRE_FALSE(server.materialGetParam(rid, "gone").has_value());

    REQUIRE(material->set("strength", 0.5));
    REQUIRE_FALSE(material->set("strength", std::array{ 1.0f, 2.0f }));
    REQUIRE_FALSE(material->set("unknown", 1.0));
    REQUIRE(std::get<float>(*server.materialGetParam(rid, "strength")) == 0.5f);

    REQUIRE(material->save(project.path("saved.material")));
    std::ifstream saved(project.root / "saved.material");
    const std::string json((std::istreambuf_iterator<char>(saved)), std::istreambuf_iterator<char>());
    std::string error;
    std::shared_ptr<Material> reloaded = Material::FromJson(json, error);
    REQUIRE(reloaded);
    REQUIRE(std::get<double>(*reloaded->get("strength")) == 0.5);
    REQUIRE(std::get<std::array<float, 4>>(*reloaded->get("tint")) == std::array{ 1.0f, 0.5f, 0.25f, 1.0f });
    REQUIRE(std::get<MaterialTextureRef>(*reloaded->get("mask")).repeat);
    REQUIRE(std::get<double>(*reloaded->get("gone")) == 3.0);

    REQUIRE_FALSE(Material::FromJson("[1, 2]", error));
}

TEST_CASE("a reloaded shader keeps its materials and their values by name", "[resource][material]")
{
    Project project;
    std::string error;
    std::shared_ptr<Material> material =
        Material::FromJson(R"({ "shader": ")" + project.path("shaders/glow.slang") + R"(", "params": { "strength": 2, "pulse": 0.25 } })", error);
    REQUIRE(material);
    std::shared_ptr<Shader> shader = material->shader();
    const MaterialRid rid = material->rid();
    const ShaderRid shader_rid = shader->rid();

    project.write("shaders/glow.slang", "import atmo;\npublic struct Glow : atmo::ICanvasMaterial {");
    REQUIRE_FALSE(shader->reload());
    REQUIRE(shader->version() == 0);

    project.write("shaders/glow.slang", glow_v2);
    REQUIRE(shader->reload());
    REQUIRE(shader->version() == 1);
    REQUIRE(shader->rid() == shader_rid);
    REQUIRE(shader->layout().params[0].name == "tint");

    REQUIRE(material->rid() == rid);
    RenderingServer &server = RenderingServer::Instance();
    REQUIRE(std::get<float>(*server.materialGetParam(rid, "strength")) == 2.0f);
    REQUIRE(std::get<float>(*server.materialGetParam(rid, "pulse")) == 0.25f);
}

TEST_CASE("exporting a project compiles its material shaders for every target", "[resource][material][export]")
{
    Project project;
    std::vector<atmo::project::ProjectManager::GeneratedFile> files;
    for (const auto &processor : atmo::project::ProjectManager::ExportProcessors())
        for (auto &file : processor(project.root)) files.push_back(std::move(file));

    REQUIRE(files.size() == 1);
    REQUIRE(files[0].path == "shaders/glow.slang.compiled");
    std::optional<ShaderDesc> desc = Shader::Deserialize({ reinterpret_cast<const char *>(files[0].data.data()), files[0].data.size() });
    REQUIRE(desc.has_value());
    REQUIRE(desc->codeFor(ShaderTarget::SpirV));
    REQUIRE(desc->codeFor(ShaderTarget::Msl));
    REQUIRE_FALSE(desc->codeFor(ShaderTarget::Dxil));
}

TEST_CASE("scripts change material parameters with material:set", "[resource][material][luau]")
{
    Project project;
    std::string error;
    std::shared_ptr<Material> material = Material::FromJson(R"({ "shader": ")" + project.path("shaders/glow.slang") + R"(" })", error);
    REQUIRE(material);

    atmo::luau::Luau vm;
    constexpr const char *script = R"(
local material = ...
assert(material:getShader() ~= "")
assert(material:set("strength", 0.75))
assert(material:get("strength") == 0.75)
assert(material:set("tint", Color(0.5, 0.25, 1, 1)))
assert(material:get("tint").g == 0.25)
assert(not material:set("missing", 1))
)";
    std::size_t size = 0;
    char *bytecode = atmo::luau::Luau::Compile(script, &size, "material_test");
    REQUIRE(bytecode);
    REQUIRE(vm.loadBytecode("material_test", bytecode, size));
    std::free(bytecode);
    atmo::luau::LuaBindings<std::shared_ptr<Material>>::Push(vm.getState(), material);
    REQUIRE(atmo::luau::Luau::ProtectedCall(vm.getState(), 1, "material_test"));

    RenderingServer &server = RenderingServer::Instance();
    REQUIRE(std::get<float>(*server.materialGetParam(material->rid(), "strength")) == 0.75f);
    REQUIRE(std::get<std::array<float, 4>>(*server.materialGetParam(material->rid(), "tint")) == std::array{ 0.5f, 0.25f, 1.0f, 1.0f });
}
