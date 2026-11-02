#pragma once

#include "gpu_platforms.hpp"

#if defined(ATMO_GPU_PLATFORMS)

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "SDL3/SDL.h"
#include "SDL3_image/SDL_image.h"
#include "offscreen_video.hpp"
#include "server/rendering/rendering_server.hpp"

namespace gpu_test
{
    using namespace atmo::server::rendering;

    constexpr int width = 128;
    constexpr int height = 96;
    inline const atmo::core::types::Color clear_color{ 0.08f, 0.09f, 0.11f, 1.0f };

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

    inline Comparison Compare(const Image &a, const Image &b, int tolerance)
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

    inline bool SavePng(const Image &image, const std::filesystem::path &path)
    {
        std::filesystem::create_directories(path.parent_path());
        SDL_Surface *surface =
            SDL_CreateSurfaceFrom(image.width, image.height, SDL_PIXELFORMAT_RGBA32, const_cast<std::uint8_t *>(image.rgba.data()), image.width * 4);
        const bool saved = surface && IMG_SavePNG(surface, path.string().c_str());
        SDL_DestroySurface(surface);
        return saved;
    }

    inline std::optional<Image> LoadPng(const std::filesystem::path &path)
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

    inline std::vector<std::byte> Checker(int size, int cell)
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

    template <typename Platform> struct GpuServer {
        GpuServer()
        {
            if (!video.available)
                return;
            REQUIRE(RenderingServer::Instance().init({ .platform = GpuPlatformTraits<Platform>::kind, .gpu_validation = true }));
            ready = RenderingServer::Instance().platformKind() == GpuPlatformTraits<Platform>::kind;
        }

        ~GpuServer()
        {
            RenderingServer::Instance().shutdown();
        }

        Image render(
            const std::function<void(RenderingServer &, CanvasRid)> &build, const Affine2d &view = {},
            const std::function<void(RenderingServer &, ViewportRid)> &setup = {})
        {
            RenderingServer &server = RenderingServer::Instance();
            CanvasRid canvas = server.canvasCreate();
            ViewportRid viewport = server.viewportCreate();
            server.viewportSetSize(viewport, { width, height });
            server.viewportSetCanvas(viewport, canvas);
            server.viewportSetClearColor(viewport, clear_color);
            server.viewportSetCanvasTransform(viewport, view);
            if (setup)
                setup(server, viewport);
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
            return dynamic_cast<Platform &>(RenderingServer::Instance().platform()).validationErrorCount();
        }

        OffscreenVideo video;
        bool ready = false;
    };

#define REQUIRE_GPU_SERVER(fixture)                                             \
    do {                                                                        \
        if (!(fixture).video.available)                                         \
            SKIP("SDL offscreen video driver unavailable: " << SDL_GetError()); \
        if (!(fixture).ready)                                                   \
            SKIP(GpuPlatformTraits<TestType>::unavailable);                     \
    } while (false)

    inline void CheckGolden(const Image &actual, const std::string &name, PlatformKind platform)
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
            SavePng(actual, std::filesystem::path("build") / "golden" / std::format("{}.{}.actual.png", name, PlatformKindName(platform)));
        CAPTURE(comparison.differing, comparison.max_difference, budget);
        REQUIRE(comparison.differing <= budget);
    }
} // namespace gpu_test

#endif
