#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/ecs/entities/2d/camera_2d/canvas_view.hpp"

using namespace atmo::core;
using atmo::core::types::Vector2;
using atmo::server::rendering::Affine2d;
using Catch::Approx;

namespace
{
    const Vector2 design{ 1280.0f, 720.0f };
    const Vector2 one{ 1.0f, 1.0f };

    void RequirePoint(Vector2 actual, Vector2 expected)
    {
        REQUIRE(actual.x == Approx(expected.x).margin(1e-3));
        REQUIRE(actual.y == Approx(expected.y).margin(1e-3));
    }
} // namespace

TEST_CASE("stretch and filter names parse", "[canvas_view]")
{
    REQUIRE(ParseStretchMode("disabled") == StretchMode::Disabled);
    REQUIRE(ParseStretchMode("fit") == StretchMode::Fit);
    REQUIRE(ParseStretchMode("integer") == StretchMode::Integer);
    REQUIRE_FALSE(ParseStretchMode("stretch").has_value());
    REQUIRE(ParseTextureFilter("nearest") == atmo::server::rendering::TextureFilter::Nearest);
    REQUIRE_FALSE(ParseTextureFilter("bilinear").has_value());
}

TEST_CASE("without stretching the canvas follows the window", "[canvas_view]")
{
    CanvasView view = ComputeCanvasView(StretchMode::Disabled, design, { 1920.0f, 1080.0f }, one, {});
    REQUIRE(view.transform == Affine2d::Identity());
    REQUIRE_FALSE(view.content.has_value());

    view = ComputeCanvasView(StretchMode::Disabled, design, { 1920.0f, 1080.0f }, one, { .active = true, .zoom = 2.0f, .position = { 100.0f, 50.0f } });
    RequirePoint(view.transform.apply({ 100.0f, 50.0f }), { 960.0f, 540.0f });
    RequirePoint(view.transform.apply({ 110.0f, 50.0f }), { 980.0f, 540.0f });
}

TEST_CASE("fit scales the design resolution and letterboxes the rest", "[canvas_view]")
{
    const CanvasView view = ComputeCanvasView(StretchMode::Fit, design, { 1920.0f, 1200.0f }, one, {});
    REQUIRE(view.content.has_value());
    RequirePoint(view.content->position, { 0.0f, 60.0f });
    RequirePoint(view.content->size, { 1920.0f, 1080.0f });
    RequirePoint(view.transform.apply({ 0.0f, 0.0f }), { 0.0f, 60.0f });
    RequirePoint(view.transform.apply({ 1280.0f, 720.0f }), { 1920.0f, 1140.0f });

    const CanvasView tall = ComputeCanvasView(StretchMode::Fit, design, { 640.0f, 720.0f }, one, {});
    RequirePoint(tall.content->position, { 0.0f, 180.0f });
    RequirePoint(tall.content->size, { 640.0f, 360.0f });
}

TEST_CASE("the same scene covers the same share of the window on 1x and 2x screens", "[canvas_view]")
{
    const CanvasView normal = ComputeCanvasView(StretchMode::Fit, design, { 1600.0f, 900.0f }, one, {});
    const CanvasView retina = ComputeCanvasView(StretchMode::Fit, design, { 1600.0f, 900.0f }, { 2.0f, 2.0f }, {});
    REQUIRE(normal.transform == retina.transform);
    RequirePoint(normal.transform.apply({ 640.0f, 360.0f }), { 800.0f, 450.0f });
}

TEST_CASE("integer stretching uses whole physical pixel scales", "[canvas_view]")
{
    const Vector2 pixel_art{ 320.0f, 180.0f };

    CanvasView view = ComputeCanvasView(StretchMode::Integer, pixel_art, { 1000.0f, 700.0f }, one, {});
    REQUIRE(view.transform.a == Approx(3.0f));
    RequirePoint(view.content->position, { 20.0f, 80.0f });
    RequirePoint(view.content->size, { 960.0f, 540.0f });

    view = ComputeCanvasView(StretchMode::Integer, pixel_art, { 1000.0f, 700.0f }, { 2.0f, 2.0f }, {});
    REQUIRE(view.transform.a * 2.0f == Approx(6.0f));
    REQUIRE(std::fmod(view.content->position.x * 2.0f, 1.0f) == Approx(0.0f));
    REQUIRE(std::fmod(view.content->position.y * 2.0f, 1.0f) == Approx(0.0f));

    view = ComputeCanvasView(StretchMode::Integer, pixel_art, { 1000.0f, 700.0f }, { 1.25f, 1.25f }, {});
    REQUIRE(view.transform.a * 1.25f == Approx(std::floor(view.transform.a * 1.25f + 1e-4f)));

    view = ComputeCanvasView(StretchMode::Integer, pixel_art, { 200.0f, 100.0f }, one, {});
    REQUIRE(view.transform.a == Approx(1.0f));
}

TEST_CASE("the camera centers on the design resolution when stretched", "[canvas_view]")
{
    const CanvasView view =
        ComputeCanvasView(StretchMode::Fit, design, { 1920.0f, 1200.0f }, one, { .active = true, .zoom = 1.0f, .position = { 500.0f, 300.0f } });
    RequirePoint(view.transform.apply({ 500.0f, 300.0f }), { 960.0f, 600.0f });
}
