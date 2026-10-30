#include "core/ecs/entities/2d/camera_2d/camera_2d.hpp"
#include "core/ecs/entities/2d/sprite_2d/sprite_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "editor/editors/scene_editor/editor_scene_context.hpp"
#include "gpu_server.hpp"

#if defined(ATMO_GPU_PLATFORMS)

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "server/rendering/shader/shader_compiler.hpp"

using namespace gpu_test;
using atmo::core::types::Color;
using atmo::core::types::Vector4;

namespace
{
    ShaderCompiler &Compiler()
    {
        static ShaderCompiler compiler;
        return compiler;
    }

    ShaderRid CreateShader(RenderingServer &server, const std::string &name, const std::string &code)
    {
        const std::array targets{ server.shaderTarget() };
        auto compiled = Compiler().compileMaterial({ .name = name, .code = code }, targets);
        INFO((compiled ? std::string() : compiled.error()));
        REQUIRE(compiled.has_value());
        return server.shaderCreate(std::move(compiled->desc));
    }

    void RequirePixel(const Image &image, int x, int y, std::array<int, 3> expected, int tolerance = 3)
    {
        const std::uint8_t *pixel = image.pixel(x, y);
        CAPTURE(x, y, int(pixel[0]), int(pixel[1]), int(pixel[2]), expected[0], expected[1], expected[2]);
        for (int c = 0; c < 3; ++c) REQUIRE(std::abs(int(pixel[c]) - expected[c]) <= tolerance);
    }

    constexpr const char *probe_source = R"(
import atmo;

public struct Probe : atmo::ICanvasMaterial
{
    float a;
    [Color] float4 tint;
    float3 v3;
    uint flag;
    float2 v2;
    int i;
    uint u;
    atmo::Texture image;

    float4 fragment(atmo::CanvasFragment input)
    {
        if (flag)
            return float4(a, v3.z, v2.y, 1.0);
        float4 t = image.sample(float2(0.5, 0.5));
        return float4(tint.r * t.r, (float(i) / 4.0) * t.g, (float(u) / 8.0) * t.b, 1.0);
    }
};
)";

    constexpr const char *custom_source = R"(
import atmo;

public struct Custom : atmo::ICanvasMaterial
{
    float4 fragment(atmo::CanvasFragment input)
    {
        return input.custom * input.color;
    }
};
)";

    constexpr const char *position_source = R"(
import atmo;

public struct Position : atmo::ICanvasMaterial
{
    float4 fragment(atmo::CanvasFragment input)
    {
        return float4(input.position / 64.0, 0.0, 1.0);
    }
};
)";

    constexpr const char *invert_source = R"(
import atmo;

public struct Invert : atmo::IViewportMaterial
{
    float4 fragment(atmo::ViewportFragment input)
    {
        float4 color = input.sample(input.uv);
        return float4(1.0 - color.rgb, 1.0);
    }
};
)";

    constexpr const char *tint_source = R"(
import atmo;

public struct Tint : atmo::IViewportMaterial
{
    [Color] float4 tint;

    float4 fragment(atmo::ViewportFragment input)
    {
        return input.sample(input.uv) * tint;
    }
};
)";

    std::string SolidSource(const char *color)
    {
        return std::string("import atmo;\npublic struct Solid : atmo::ICanvasMaterial\n{\n    float4 fragment(atmo::CanvasFragment input) { return float4(") +
            color + "); }\n};\n";
    }
} // namespace

TEMPLATE_TEST_CASE("material parameters of every type reach the shader", "[rendering][gpu][material]", ATMO_GPU_PLATFORMS)
{
    GpuServer<TestType> gpu;
    REQUIRE_GPU_SERVER(gpu);
    RenderingServer &server = RenderingServer::Instance();

    const ShaderRid shader = CreateShader(server, "probe", probe_source);
    const MaterialRid flagged = server.materialCreate(shader);
    REQUIRE(server.materialSetParam(flagged, "flag", true));
    REQUIRE(server.materialSetParam(flagged, "a", 0.25f));
    REQUIRE(server.materialSetParam(flagged, "v3", std::array{ 0.0f, 0.0f, 0.5f }));
    REQUIRE(server.materialSetParam(flagged, "v2", std::array{ 0.0f, 0.75f }));

    const std::array<std::byte, 4> cyan{ std::byte{ 0x80 }, std::byte{ 0xFF }, std::byte{ 0xFF }, std::byte{ 0xFF } };
    const TextureRid texture = server.texture2dCreate({ .size = { 1, 1 } }, cyan);
    const MaterialRid textured = server.materialCreate(shader);
    REQUIRE(server.materialSetParam(textured, "tint", std::array{ 1.0f, 1.0f, 1.0f, 1.0f }));
    REQUIRE(server.materialSetParam(textured, "i", 2));
    REQUIRE(server.materialSetParam(textured, "u", 4U));
    REQUIRE(server.materialSetParam(textured, "image", MaterialTexture{ .texture = texture, .filter = TextureFilter::Nearest }));

    const MaterialRid untextured = server.materialCreate(shader);
    REQUIRE(server.materialSetParam(untextured, "i", 4.0f));
    REQUIRE(server.materialSetParam(untextured, "u", 8));

    REQUIRE_FALSE(server.materialSetParam(flagged, "missing", 1.0f));
    REQUIRE_FALSE(server.materialSetParam(flagged, "v2", 1.0f));

    const Image image = gpu.render([&](RenderingServer &server, CanvasRid canvas) {
        const std::array<MaterialRid, 3> materials{ flagged, textured, untextured };
        for (std::size_t i = 0; i < materials.size(); ++i) {
            CanvasItemRid item = server.canvasItemCreate(canvas);
            server.canvasItemSetMaterial(item, materials[i]);
            server.canvasItemAddRect(item, { { 8.0f + 40.0f * static_cast<float>(i), 8.0f }, { 32.0f, 32.0f } }, Color(1.0f, 1.0f, 1.0f, 1.0f));
        }
        CanvasItemRid plain = server.canvasItemCreate(canvas);
        server.canvasItemAddRect(plain, { { 8.0f, 56.0f }, { 32.0f, 32.0f } }, Color(0.0f, 1.0f, 0.0f, 1.0f));
    });

    RequirePixel(image, 24, 24, { 64, 128, 191 });
    RequirePixel(image, 64, 24, { 128, 128, 128 });
    RequirePixel(image, 104, 24, { 255, 255, 255 });
    RequirePixel(image, 24, 72, { 0, 255, 0 });
    RequirePixel(image, 70, 72, { 20, 23, 28 });

    server.materialFree(flagged);
    server.materialFree(textured);
    server.materialFree(untextured);
    server.shaderFree(shader);
    server.textureFree(texture);
    REQUIRE(gpu.validationErrors() == 0);
}

TEMPLATE_TEST_CASE("materials read the item custom value, the modulate and the canvas position", "[rendering][gpu][material]", ATMO_GPU_PLATFORMS)
{
    GpuServer<TestType> gpu;
    REQUIRE_GPU_SERVER(gpu);
    RenderingServer &server = RenderingServer::Instance();

    const ShaderRid custom_shader = CreateShader(server, "custom", custom_source);
    const MaterialRid custom = server.materialCreate(custom_shader);
    const ShaderRid position_shader = CreateShader(server, "position", position_source);
    const MaterialRid position = server.materialCreate(position_shader);

    const Image image = gpu.render(
        [&](RenderingServer &server, CanvasRid canvas) {
            CanvasItemRid tinted = server.canvasItemCreate(canvas);
            server.canvasItemSetMaterial(tinted, custom);
            server.canvasItemSetCustom(tinted, Vector4{ 0.5f, 0.25f, 1.0f, 1.0f });
            server.canvasItemSetModulate(tinted, Color(1.0f, 1.0f, 0.5f, 1.0f));
            server.canvasItemAddRect(tinted, { { 2.0f, 2.0f }, { 16.0f, 16.0f } }, Color(1.0f, 1.0f, 1.0f, 1.0f));

            CanvasItemRid located = server.canvasItemCreate(canvas);
            server.canvasItemSetMaterial(located, position);
            server.canvasItemAddRect(located, { { 16.0f, 0.0f }, { 40.0f, 40.0f } }, Color(1.0f, 1.0f, 1.0f, 1.0f));
        },
        Affine2d::Scaling({ 2.0f, 2.0f }));

    RequirePixel(image, 10, 10, { 128, 64, 128 });
    RequirePixel(image, 40, 20, { 81, 41, 0 });

    server.materialFree(custom);
    server.materialFree(position);
    server.shaderFree(custom_shader);
    server.shaderFree(position_shader);
    REQUIRE(gpu.validationErrors() == 0);
}

TEMPLATE_TEST_CASE("post-processing materials run in order and leave positive canvas layers alone", "[rendering][gpu][material]", ATMO_GPU_PLATFORMS)
{
    GpuServer<TestType> gpu;
    REQUIRE_GPU_SERVER(gpu);
    RenderingServer &server = RenderingServer::Instance();

    const ShaderRid invert_shader = CreateShader(server, "invert", invert_source);
    const MaterialRid invert = server.materialCreate(invert_shader);
    const ShaderRid tint_shader = CreateShader(server, "tint", tint_source);
    const MaterialRid tint = server.materialCreate(tint_shader);
    REQUIRE(server.materialSetParam(tint, "tint", std::array{ 1.0f, 0.5f, 1.0f, 1.0f }));
    REQUIRE(server.shaderGetKind(tint_shader) == ShaderKind::Viewport);

    CanvasRid overlay = server.canvasCreate();
    CanvasItemRid label = server.canvasItemCreate(overlay);
    server.canvasItemAddRect(label, { { 100.0f, 70.0f }, { 20.0f, 20.0f } }, Color(0.0f, 0.0f, 1.0f, 1.0f));

    const Image image = gpu.render(
        [&](RenderingServer &server, CanvasRid canvas) {
            CanvasItemRid item = server.canvasItemCreate(canvas);
            server.canvasItemAddRect(item, { { 8.0f, 8.0f }, { 32.0f, 32.0f } }, Color(1.0f, 0.0f, 0.0f, 1.0f));
        },
        {},
        [&](RenderingServer &server, ViewportRid viewport) {
            const std::array chain{ invert, tint };
            server.viewportSetPostProcess(viewport, chain);
            server.viewportAttachCanvas(viewport, overlay, 1);
        });

    RequirePixel(image, 24, 24, { 0, 128, 255 });
    RequirePixel(image, 64, 48, { 235, 116, 227 });
    RequirePixel(image, 110, 80, { 0, 0, 255 });

    server.canvasFree(overlay);
    server.materialFree(invert);
    server.materialFree(tint);
    server.shaderFree(invert_shader);
    server.shaderFree(tint_shader);
    REQUIRE(gpu.validationErrors() == 0);
}

TEMPLATE_TEST_CASE("updating a shader keeps its materials and their parameters", "[rendering][gpu][material]", ATMO_GPU_PLATFORMS)
{
    GpuServer<TestType> gpu;
    REQUIRE_GPU_SERVER(gpu);
    RenderingServer &server = RenderingServer::Instance();

    const ShaderRid shader = CreateShader(server, "solid", SolidSource("1.0, 0.0, 0.0, 1.0"));
    const MaterialRid material = server.materialCreate(shader);

    const auto draw = [&](RenderingServer &server, CanvasRid canvas) {
        CanvasItemRid item = server.canvasItemCreate(canvas);
        server.canvasItemSetMaterial(item, material);
        server.canvasItemAddRect(item, { { 0.0f, 0.0f }, { 32.0f, 32.0f } }, Color(1.0f, 1.0f, 1.0f, 1.0f));
    };
    RequirePixel(gpu.render(draw), 16, 16, { 255, 0, 0 });

    const std::array targets{ server.shaderTarget() };
    auto updated = Compiler().compileMaterial(
        { .name = "solid",
          .code = "import atmo;\npublic struct Solid : atmo::ICanvasMaterial\n{\n    [Color] float4 color;\n    float4 fragment(atmo::CanvasFragment "
                  "input) { return color * input.color; }\n};\n" },
        targets);
    REQUIRE(updated.has_value());
    server.shaderUpdate(shader, std::move(updated->desc));
    REQUIRE(server.materialSetParam(material, "color", std::array{ 0.0f, 1.0f, 0.0f, 1.0f }));
    RequirePixel(gpu.render(draw), 16, 16, { 0, 255, 0 });

    ShaderDesc missing{ .name = "missing", .entry_point = "materialFragmentMain" };
    server.shaderUpdate(shader, std::move(missing));
    RequirePixel(gpu.render(draw), 16, 16, { 255, 255, 255 });

    server.materialFree(material);
    server.shaderFree(shader);
    REQUIRE(gpu.validationErrors() == 0);
}

TEMPLATE_TEST_CASE("sprites and cameras apply their materials, modulate and custom value in a world", "[rendering][gpu][material]", ATMO_GPU_PLATFORMS)
{
    GpuServer<TestType> gpu;
    REQUIRE_GPU_SERVER(gpu);
    RenderingServer &server = RenderingServer::Instance();

    const std::filesystem::path root = std::filesystem::temp_directory_path() / "atmo-material-world-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const auto write = [&root](const std::string &name, const std::string &content) {
        std::ofstream out(root / name, std::ios::binary | std::ios::trunc);
        out << content;
        return (root / name).generic_string();
    };

    Image white{ .width = 16, .height = 16, .rgba = std::vector<std::uint8_t>(16 * 16 * 4, 0xFF) };
    REQUIRE(SavePng(white, root / "white.png"));
    const std::string tint_shader = write(
        "tint.slang",
        "import atmo;\npublic struct Tint : atmo::ICanvasMaterial\n{\n    [Color] float4 tint;\n    float4 fragment(atmo::CanvasFragment input)\n    {\n"
        "        return input.sample(input.uv) * input.color * tint + float4(input.custom.rgb, 0.0);\n    }\n};\n");
    const std::string invert_shader = write("invert.slang", invert_source);
    const std::string tint_material = write("tint.material", R"({ "shader": ")" + tint_shader + R"(", "params": { "tint": [1, 1, 0, 1] } })");
    const std::string invert_material = write("invert.material", R"({ "shader": ")" + invert_shader + R"(" })");

    {
        atmo::editor::EditorSceneContext context;
        context.init({ 1.0f, 1.0f });
        REQUIRE(context.isReady());
        context.resize(width, height);

        auto sprite = std::static_pointer_cast<atmo::core::ecs::entities::Sprite2d>(
            atmo::core::ecs::EntityRegistry::CreateIn(&context.getWorld(), "Entity::Entity2d::Sprite2d"));
        REQUIRE(sprite);
        sprite->setParent(*context.getScene());
        sprite->setPosition({ -30.0f, -20.0f });
        sprite->setTexturePath((root / "white.png").string());
        sprite->setMaterialPath(tint_material);
        sprite->setModulate(Color(0.5f, 1.0f, 1.0f, 1.0f));
        sprite->setCustom({ 0.0f, 0.0f, 0.5f, 0.0f });

        auto camera = std::static_pointer_cast<atmo::core::ecs::entities::Camera2d>(
            atmo::core::ecs::EntityRegistry::CreateIn(&context.getWorld(), "Entity::Entity2d::Camera2d"));
        REQUIRE(camera);
        camera->setParent(*context.getScene());

        const auto capture = [&] {
            for (int frame = 0; frame < 2; ++frame) {
                context.tick(1.0f / 60.0f);
                server.sync();
            }
            std::optional<TextureData> data;
            server.textureRequestData(context.getViewportTexture(), [&data](TextureData result) { data = std::move(result); });
            server.sync();
            server.waitIdle();
            REQUIRE(data.has_value());
            REQUIRE(data->size.x == width);
            Image image{ .width = width, .height = height, .rgba = std::vector<std::uint8_t>(data->pixels.size()) };
            std::memcpy(image.rgba.data(), data->pixels.data(), data->pixels.size());
            return image;
        };

        const atmo::core::types::Vector2 center = context.worldToScreen({ -30.0f, -20.0f });
        RequirePixel(capture(), static_cast<int>(center.x), static_cast<int>(center.y), { 128, 255, 128 });

        camera->setPostProcess({ invert_material });
        RequirePixel(capture(), static_cast<int>(center.x), static_cast<int>(center.y), { 127, 0, 127 });

        camera->setPostProcess({});
        sprite->setMaterialPath("");
        RequirePixel(capture(), static_cast<int>(center.x), static_cast<int>(center.y), { 128, 255, 255 });
    }

    std::filesystem::remove_all(root);
    REQUIRE(gpu.validationErrors() == 0);
}

#endif
