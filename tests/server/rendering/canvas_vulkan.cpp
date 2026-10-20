#if defined(ATMO_RENDERING_VULKAN)

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <clay.h>

#include "SDL3/SDL.h"
#include "SDL3_image/SDL_image.h"
#include "core/resource/font.hpp"
#include "impl/clay_canvas_renderer.hpp"
#include "offscreen_video.hpp"
#include "server/rendering/api/vulkan/vulkan_platform.hpp"
#include "server/rendering/rendering_server.hpp"

using namespace atmo::server::rendering;
using atmo::core::types::Color;
using atmo::core::types::Vector2;

namespace
{
    constexpr int width = 128;
    constexpr int height = 96;
    const Color clear_color{ 0.08f, 0.09f, 0.11f, 1.0f };

    struct Image {
        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> rgba;

        [[nodiscard]] const std::uint8_t *pixel(int x, int y) const
        {
            return rgba.data() + ((static_cast<std::size_t>(y) * width) + x) * 4;
        }
    };

    struct Comparison {
        std::size_t differing = 0;
        int max_difference = 0;
    };

    Comparison Compare(const Image &a, const Image &b, int tolerance)
    {
        Comparison result;
        for (int y = 0; y < a.height; ++y) {
            for (int x = 0; x < a.width; ++x) {
                int worst = 0;
                for (int c = 0; c < 3; ++c) worst = std::max(worst, std::abs(int(a.pixel(x, y)[c]) - int(b.pixel(x, y)[c])));
                result.max_difference = std::max(result.max_difference, worst);
                if (worst > tolerance)
                    ++result.differing;
            }
        }
        return result;
    }

    bool SavePng(const Image &image, const std::filesystem::path &path)
    {
        std::filesystem::create_directories(path.parent_path());
        SDL_Surface *surface =
            SDL_CreateSurfaceFrom(image.width, image.height, SDL_PIXELFORMAT_RGBA32, const_cast<std::uint8_t *>(image.rgba.data()), image.width * 4);
        const bool saved = surface && IMG_SavePNG(surface, path.string().c_str());
        SDL_DestroySurface(surface);
        return saved;
    }

    std::optional<Image> LoadPng(const std::filesystem::path &path)
    {
        SDL_Surface *loaded = IMG_Load(path.string().c_str());
        if (!loaded)
            return std::nullopt;
        SDL_Surface *surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        if (!surface)
            return std::nullopt;

        Image image{ .width = surface->w, .height = surface->h, .rgba = std::vector<std::uint8_t>(static_cast<std::size_t>(surface->w * surface->h * 4)) };
        for (int y = 0; y < surface->h; ++y)
            std::memcpy(
                image.rgba.data() + (static_cast<std::size_t>(y) * surface->w * 4),
                static_cast<const std::uint8_t *>(surface->pixels) + (static_cast<std::size_t>(y) * surface->pitch),
                static_cast<std::size_t>(surface->w) * 4);
        SDL_DestroySurface(surface);
        return image;
    }

    std::vector<std::byte> Checker(int size, int cell)
    {
        std::vector<std::byte> pixels(static_cast<std::size_t>(size * size * 4));
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                const bool light = ((x / cell) + (y / cell)) % 2 == 0;
                std::byte *p = pixels.data() + ((static_cast<std::size_t>(y) * size) + x) * 4;
                p[0] = std::byte{ static_cast<std::uint8_t>(light ? 0xF2 : 0x2E) };
                p[1] = std::byte{ static_cast<std::uint8_t>(light ? 0xA6 : 0x6F) };
                p[2] = std::byte{ static_cast<std::uint8_t>(light ? 0x3B : 0xB5) };
                p[3] = std::byte{ 0xFF };
            }
        }
        return pixels;
    }

    struct Sprite {
        Vector2 position;
        float rotation;
        Vector2 scale;
    };

    const std::array sprites{
        Sprite{ { 30.0f, 30.0f }, 0.0f, { 1.0f, 1.0f } },
        Sprite{ { 90.0f, 28.0f }, 30.0f, { 1.5f, 1.0f } },
        Sprite{ { 40.0f, 70.0f }, -15.0f, { 0.75f, 0.75f } },
        Sprite{ { 96.0f, 72.0f }, 90.0f, { 1.0f, 2.0f } },
    };
    constexpr float camera_zoom = 1.25f;
    const Vector2 camera_position{ 60.0f, 50.0f };
    constexpr int sprite_size = 16;

    struct VulkanServer {
        VulkanServer()
        {
            if (!video.available)
                return;
            REQUIRE(RenderingServer::Instance().init({ .platform = PlatformKind::Vulkan, .gpu_validation = true }));
            ready = RenderingServer::Instance().platformKind() == PlatformKind::Vulkan;
        }

        ~VulkanServer()
        {
            RenderingServer::Instance().shutdown();
        }

        Image render(const std::function<void(RenderingServer &, CanvasRid)> &build, const Affine2d &view = {})
        {
            RenderingServer &server = RenderingServer::Instance();
            CanvasRid canvas = server.canvasCreate();
            ViewportRid viewport = server.viewportCreate();
            server.viewportSetSize(viewport, { width, height });
            server.viewportSetCanvas(viewport, canvas);
            server.viewportSetClearColor(viewport, clear_color);
            server.viewportSetCanvasTransform(viewport, view);
            build(server, canvas);

            std::optional<TextureData> data;
            server.textureRequestData(server.viewportGetTexture(viewport), [&data](TextureData result) { data = std::move(result); });
            server.sync();
            server.waitIdle();
            REQUIRE(data.has_value());
            REQUIRE(data->pixels.size() == static_cast<std::size_t>(width * height * 4));

            server.viewportFree(viewport);
            server.canvasFree(canvas);

            Image image{ .width = width, .height = height, .rgba = std::vector<std::uint8_t>(data->pixels.size()) };
            std::memcpy(image.rgba.data(), data->pixels.data(), data->pixels.size());
            return image;
        }

        [[nodiscard]] std::uint32_t validationErrors()
        {
            return dynamic_cast<VulkanPlatform &>(RenderingServer::Instance().platform()).validationErrorCount();
        }

        OffscreenVideo video;
        bool ready = false;
    };

#define REQUIRE_VULKAN_SERVER(fixture)                                          \
    do {                                                                        \
        if (!(fixture).video.available)                                         \
            SKIP("SDL offscreen video driver unavailable: " << SDL_GetError()); \
        if (!(fixture).ready)                                                   \
            SKIP("No Vulkan 1.3 device with the required features");            \
    } while (false)

    void CheckGolden(const Image &actual, const std::string &name)
    {
        const std::filesystem::path golden = std::filesystem::path("tests") / "golden" / (name + ".png");
        if (std::getenv("ATMO_UPDATE_GOLDENS")) {
            REQUIRE(SavePng(actual, golden));
            WARN("Updated golden image " << golden.string());
            return;
        }

        std::optional<Image> expected = LoadPng(golden);
        REQUIRE(expected.has_value());
        REQUIRE(expected->width == actual.width);
        REQUIRE(expected->height == actual.height);

        const Comparison comparison = Compare(actual, *expected, 8);
        const std::size_t budget = static_cast<std::size_t>(actual.width * actual.height) / 200;
        if (comparison.differing > budget)
            SavePng(actual, std::filesystem::path("build") / "golden" / (name + ".actual.png"));
        CAPTURE(comparison.differing, comparison.max_difference, budget);
        REQUIRE(comparison.differing <= budget);
    }
} // namespace

TEST_CASE("the canvas renderer matches its golden image", "[rendering][vulkan][golden]")
{
    VulkanServer vulkan;
    REQUIRE_VULKAN_SERVER(vulkan);

    TextureRid checker;
    const Image image = vulkan.render([&checker](RenderingServer &server, CanvasRid canvas) {
        checker = server.texture2dCreate({ .size = { 16, 16 } }, Checker(16, 4));

        CanvasItemRid background = server.canvasItemCreate(canvas);
        server.canvasItemSetZIndex(background, -1);
        server.canvasItemAddRect(background, { { 4.0f, 4.0f }, { 56.0f, 40.0f } }, Color(0.25f, 0.5f, 0.9f, 1.0f));
        server.canvasItemAddRect(background, { { 24.0f, 20.0f }, { 56.0f, 40.0f } }, Color(0.95f, 0.3f, 0.3f, 0.5f));

        CanvasItemRid sprite = server.canvasItemCreate(canvas);
        server.canvasItemSetTransform(sprite, Affine2d::FromTrs({ 96.0f, 30.0f }, 30.0f, { 2.0f, 1.5f }));
        server.canvasItemAddTextureRect(sprite, Rect2::Centered({ 16.0f, 16.0f }), checker, { .filter = TextureFilter::Nearest });

        CanvasItemRid shapes = server.canvasItemCreate(canvas);
        const std::array<Vector2, 3> triangle{ Vector2(10.0f, 90.0f), Vector2(50.0f, 90.0f), Vector2(30.0f, 60.0f) };
        const std::array<Color, 3> colors{ Color(1.0f, 0.0f, 0.0f, 1.0f), Color(0.0f, 1.0f, 0.0f, 1.0f), Color(0.0f, 0.0f, 1.0f, 1.0f) };
        server.canvasItemAddPolygon(shapes, triangle, colors);
        const std::array<Vector2, 4> line{ Vector2(60.0f, 90.0f), Vector2(80.0f, 60.0f), Vector2(100.0f, 90.0f), Vector2(120.0f, 60.0f) };
        server.canvasItemAddPolyline(shapes, line, Color(1.0f, 1.0f, 1.0f, 1.0f), 2.0f);

        CanvasItemRid clipped = server.canvasItemCreate(canvas);
        server.canvasItemPushClip(clipped, { { 64.0f, 48.0f }, { 16.0f, 16.0f } });
        server.canvasItemAddRect(clipped, { { 56.0f, 40.0f }, { 40.0f, 40.0f } }, Color(1.0f, 0.85f, 0.2f, 1.0f));
        server.canvasItemPopClip(clipped);
    });

    CheckGolden(image, "canvas_basic");
    RenderingServer::Instance().textureFree(checker);
    REQUIRE(vulkan.validationErrors() == 0);
}

TEST_CASE("sprites land where the SDL_Renderer path drew them", "[rendering][vulkan][parity]")
{
    VulkanServer vulkan;
    REQUIRE_VULKAN_SERVER(vulkan);

    const std::vector<std::byte> pixels = Checker(sprite_size, 4);

    SDL_Surface *surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    REQUIRE(surface != nullptr);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    REQUIRE(renderer != nullptr);
    SDL_Surface *sprite_surface =
        SDL_CreateSurfaceFrom(sprite_size, sprite_size, SDL_PIXELFORMAT_RGBA32, const_cast<std::byte *>(pixels.data()), sprite_size * 4);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, sprite_surface);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderDrawColorFloat(renderer, clear_color.r, clear_color.g, clear_color.b, 1.0f);
    SDL_RenderClear(renderer);

    for (const Sprite &sprite : sprites) {
        const float sx = (sprite.position.x - camera_position.x) * camera_zoom + static_cast<float>(width) * 0.5f;
        const float sy = (sprite.position.y - camera_position.y) * camera_zoom + static_cast<float>(height) * 0.5f;
        const float w = sprite_size * sprite.scale.x * camera_zoom;
        const float h = sprite_size * sprite.scale.y * camera_zoom;
        const SDL_FRect dst{ sx - w * 0.5f, sy - h * 0.5f, w, h };
        const SDL_FPoint center{ dst.w * 0.5f, dst.h * 0.5f };
        SDL_RenderTextureRotated(renderer, texture, nullptr, &dst, sprite.rotation, &center, SDL_FLIP_NONE);
    }
    SDL_RenderPresent(renderer);

    Image expected{ .width = width, .height = height, .rgba = std::vector<std::uint8_t>(static_cast<std::size_t>(width * height * 4)) };
    for (int y = 0; y < height; ++y)
        std::memcpy(
            expected.rgba.data() + (static_cast<std::size_t>(y) * width * 4),
            static_cast<const std::uint8_t *>(surface->pixels) + (static_cast<std::size_t>(y) * surface->pitch),
            static_cast<std::size_t>(width) * 4);
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(sprite_surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);

    const Affine2d view = Affine2d::Translation({ width * 0.5f, height * 0.5f }) * Affine2d::Scaling({ camera_zoom, camera_zoom }) *
        Affine2d::Translation({ -camera_position.x, -camera_position.y });

    TextureRid sprite_texture;
    const Image actual = vulkan.render(
        [&](RenderingServer &server, CanvasRid canvas) {
            sprite_texture = server.texture2dCreate({ .size = { sprite_size, sprite_size } }, pixels);
            for (const Sprite &sprite : sprites) {
                CanvasItemRid item = server.canvasItemCreate(canvas);
                server.canvasItemSetTransform(item, Affine2d::FromTrs(sprite.position, sprite.rotation, sprite.scale));
                server.canvasItemAddTextureRect(
                    item, Rect2::Centered({ float(sprite_size), float(sprite_size) }), sprite_texture, { .filter = TextureFilter::Nearest });
            }
        },
        view);

    const Comparison comparison = Compare(actual, expected, 48);
    const std::size_t budget = static_cast<std::size_t>(width * height) / 50;
    if (comparison.differing > budget) {
        SavePng(actual, std::filesystem::path("build") / "golden" / "sprite_parity.actual.png");
        SavePng(expected, std::filesystem::path("build") / "golden" / "sprite_parity.expected.png");
    }
    CAPTURE(comparison.differing, comparison.max_difference, budget);
    REQUIRE(comparison.differing <= budget);

    RenderingServer::Instance().textureFree(sprite_texture);
    REQUIRE(vulkan.validationErrors() == 0);
}

TEST_CASE("letterbox bars use the border color and clip the canvas", "[rendering][vulkan]")
{
    VulkanServer vulkan;
    REQUIRE_VULKAN_SERVER(vulkan);

    RenderingServer &server = RenderingServer::Instance();
    CanvasRid canvas = server.canvasCreate();
    ViewportRid viewport = server.viewportCreate();
    server.viewportSetSize(viewport, { width, height });
    server.viewportSetCanvas(viewport, canvas);
    server.viewportSetClearColor(viewport, Color(0.0f, 0.0f, 1.0f, 1.0f));
    server.viewportSetBorderColor(viewport, Color(0.0f, 1.0f, 0.0f, 1.0f));
    server.viewportSetContentRect(viewport, Rect2{ { 32.0f, 0.0f }, { 64.0f, float(height) } });
    CanvasItemRid item = server.canvasItemCreate(canvas);
    server.canvasItemAddRect(item, { { 0.0f, 0.0f }, { float(width), 10.0f } }, Color(1.0f, 0.0f, 0.0f, 1.0f));

    std::optional<TextureData> data;
    server.textureRequestData(server.viewportGetTexture(viewport), [&data](TextureData result) { data = std::move(result); });
    server.sync();
    server.waitIdle();
    REQUIRE(data.has_value());

    auto pixel = [&data](int x, int y) {
        const std::size_t offset = ((static_cast<std::size_t>(y) * width) + x) * 4;
        return std::array<int, 3>{ int(data->pixels[offset]), int(data->pixels[offset + 1]), int(data->pixels[offset + 2]) };
    };
    REQUIRE(pixel(4, 50) == std::array<int, 3>{ 0, 255, 0 });
    REQUIRE(pixel(4, 4) == std::array<int, 3>{ 0, 255, 0 });
    REQUIRE(pixel(64, 50) == std::array<int, 3>{ 0, 0, 255 });
    REQUIRE(pixel(64, 4) == std::array<int, 3>{ 255, 0, 0 });

    server.viewportFree(viewport);
    server.canvasFree(canvas);
    REQUIRE(vulkan.validationErrors() == 0);
}

TEST_CASE("a Clay layout matches its golden image", "[rendering][vulkan][golden][ui]")
{
    VulkanServer vulkan;
    REQUIRE_VULKAN_SERVER(vulkan);

    RenderingServer &server = RenderingServer::Instance();
    std::ifstream file("assets/fonts/Nunito/Nunito.ttf", std::ios::binary);
    const std::vector<char> chars{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    REQUIRE_FALSE(chars.empty());
    atmo::core::resource::Font font(server.fontCreate(std::as_bytes(std::span(chars)), "Nunito"));
    REQUIRE(font.rid().isValid());
    const TextureRid checker = server.texture2dCreate({ .size = { 16, 16 } }, Checker(16, 4));

    const std::uint32_t memory_size = Clay_MinMemorySize();
    void *memory = std::malloc(memory_size);
    Clay_Context *previous = Clay_GetCurrentContext();
    Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(memory_size, memory), { float(width), float(height) }, {});
    Clay_SetMeasureTextFunction(atmo::impl::ClayMeasureText, nullptr);

    const auto open = [](const Clay_ElementDeclaration &declaration) {
        Clay__OpenElement();
        Clay__ConfigureOpenElement(declaration);
    };

    Clay_BeginLayout();
    open(
        {
            .id = CLAY_ID("Root"),
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                        .padding = { 6, 6, 6, 6 },
                        .childGap = 6,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM },
            .backgroundColor = { 30, 34, 42, 255 },
        });
    open(
        {
            .id = CLAY_ID("Button"),
            .layout = { .sizing = { CLAY_SIZING_FIXED(84), CLAY_SIZING_FIXED(30) }, .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = { 59, 130, 246, 255 },
            .cornerRadius = { 10, 10, 10, 10 },
            .border = { .color = { 255, 255, 255, 255 }, .width = { 2, 2, 2, 2, 0 } },
        });
    CLAY_TEXT(CLAY_STRING("Atmo"), CLAY_TEXT_CONFIG({ .userData = &font, .textColor = { 255, 255, 255, 255 }, .fontSize = 16 }));
    Clay__CloseElement();
    open(
        {
            .id = CLAY_ID("Clip"),
            .layout = { .sizing = { CLAY_SIZING_FIXED(64), CLAY_SIZING_FIXED(30) } },
            .backgroundColor = { 200, 60, 60, 255 },
            .clip = { .horizontal = true, .vertical = true },
        });
    open(
        {
            .id = CLAY_ID("Image"),
            .layout = { .sizing = { CLAY_SIZING_FIXED(48), CLAY_SIZING_FIXED(48) } },
            .image = { .imageData = const_cast<TextureRid *>(&checker) },
        });
    Clay__CloseElement();
    Clay__CloseElement();
    Clay__CloseElement();
    const Clay_RenderCommandArray commands = Clay_EndLayout();

    CanvasCommandWriter writer;
    atmo::impl::ClayRenderCommands(commands, writer, 1.0f);
    REQUIRE(writer.size() == 8);
    Clay_SetCurrentContext(previous);
    std::free(memory);

    const Image image = vulkan.render([&writer](RenderingServer &target, CanvasRid canvas) {
        const CanvasItemRid item = target.canvasItemCreate(canvas);
        target.canvasItemSetCommands(item, std::move(writer));
    });

    CheckGolden(image, "ui_basic");
    server.textureFree(checker);
    REQUIRE(vulkan.validationErrors() == 0);
}

#endif
