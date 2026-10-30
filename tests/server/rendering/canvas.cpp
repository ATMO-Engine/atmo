#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "server/rendering/api/null/null_platform.hpp"
#include "server/rendering/renderer/render_graph.hpp"
#include "server/rendering/rendering_server.hpp"

using namespace atmo::server::rendering;
using atmo::core::types::Color;
using atmo::core::types::Vector2;

namespace
{
    struct CanvasFixture {
        explicit CanvasFixture(RenderingSettings settings = { .platform = PlatformKind::Null, .render_thread = false })
        {
            REQUIRE(server.init(settings));
            canvas = server.canvasCreate();
            viewport = server.viewportCreate();
            server.viewportSetSize(viewport, { 100, 100 });
            server.viewportSetCanvas(viewport, canvas);
        }

        ~CanvasFixture()
        {
            server.shutdown();
        }

        FrameStats frame()
        {
            server.sync();
            server.waitIdle();
            return server.lastFrameStats();
        }

        RenderingServer &server = RenderingServer::Instance();
        CanvasRid canvas;
        ViewportRid viewport;
    };

    const Color red{ 1.0f, 0.0f, 0.0f, 1.0f };
    const Color green{ 0.0f, 1.0f, 0.0f, 1.0f };
} // namespace

TEST_CASE("rects in one canvas batch into a single draw", "[rendering][canvas]")
{
    CanvasFixture f;
    for (int i = 0; i < 3; ++i) {
        CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
        f.server.canvasItemAddRect(item, { { static_cast<float>(i * 10), 0.0f }, { 10.0f, 10.0f } }, red);
    }

    const FrameStats stats = f.frame();
    REQUIRE(stats.canvas_items == 3);
    REQUIRE(stats.instances == 3);
    REQUIRE(stats.draw_calls == 1);
}

TEST_CASE("sprites from different textures still share one draw", "[rendering][canvas]")
{
    CanvasFixture f;
    std::vector<std::byte> pixels(4 * 4 * 4, std::byte{ 0xFF });
    for (int i = 0; i < 8; ++i) {
        TextureRid texture = f.server.texture2dCreate({ .size = { 4, 4 } }, pixels);
        CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
        f.server.canvasItemAddTextureRect(item, { { 10.0f * static_cast<float>(i), 10.0f }, { 8.0f, 8.0f } }, texture);
    }

    const FrameStats stats = f.frame();
    REQUIRE(stats.instances == 8);
    REQUIRE(stats.draw_calls == 1);
}

TEST_CASE("geometry kinds keep their painter order", "[rendering][canvas]")
{
    CanvasFixture f;
    CanvasItemRid first = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(first, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, red);

    CanvasItemRid polygon = f.server.canvasItemCreate(f.canvas);
    const std::array<Vector2, 3> triangle{ Vector2(0.0f, 0.0f), Vector2(20.0f, 0.0f), Vector2(0.0f, 20.0f) };
    f.server.canvasItemAddPolygon(polygon, triangle, { &green, 1 });

    CanvasItemRid last = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(last, { { 20.0f, 20.0f }, { 10.0f, 10.0f } }, red);

    FrameStats stats = f.frame();
    REQUIRE(stats.draw_calls == 3);
    REQUIRE(stats.vertices == 3);

    f.server.canvasItemSetZIndex(polygon, 5);
    stats = f.frame();
    REQUIRE(stats.draw_calls == 2);
}

TEST_CASE("items outside the viewport are culled", "[rendering][canvas]")
{
    CanvasFixture f;
    CanvasItemRid visible = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(visible, { { 10.0f, 10.0f }, { 10.0f, 10.0f } }, red);

    CanvasItemRid outside = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(outside, { { 500.0f, 500.0f }, { 10.0f, 10.0f } }, red);

    FrameStats stats = f.frame();
    REQUIRE(stats.instances == 1);
    REQUIRE(stats.culled_items == 1);

    f.server.viewportSetCanvasTransform(f.viewport, Affine2d::Translation({ -495.0f, -495.0f }));
    stats = f.frame();
    REQUIRE(stats.instances == 1);
    REQUIRE(stats.culled_items == 1);
}

TEST_CASE("clip rects split batches and cull what they hide", "[rendering][canvas]")
{
    CanvasFixture f;
    CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(item, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, red);
    f.server.canvasItemPushClip(item, { { 0.0f, 0.0f }, { 50.0f, 50.0f } });
    f.server.canvasItemAddRect(item, { { 10.0f, 10.0f }, { 10.0f, 10.0f } }, red);
    f.server.canvasItemAddRect(item, { { 60.0f, 60.0f }, { 10.0f, 10.0f } }, red);
    f.server.canvasItemPopClip(item);
    f.server.canvasItemAddRect(item, { { 60.0f, 60.0f }, { 10.0f, 10.0f } }, red);

    const FrameStats stats = f.frame();
    REQUIRE(stats.instances == 3);
    REQUIRE(stats.culled_items == 1);
    REQUIRE(stats.draw_calls == 3);
}

TEST_CASE("hidden parents hide their children", "[rendering][canvas]")
{
    CanvasFixture f;
    CanvasItemRid parent = f.server.canvasItemCreate(f.canvas);
    CanvasItemRid child = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemSetParent(child, parent);
    f.server.canvasItemAddRect(child, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, red);

    REQUIRE(f.frame().instances == 1);

    f.server.canvasItemSetVisible(parent, false);
    REQUIRE(f.frame().instances == 0);

    f.server.canvasItemSetVisible(parent, true);
    f.server.canvasItemSetTransform(parent, Affine2d::Translation({ 1000.0f, 0.0f }));
    const FrameStats stats = f.frame();
    REQUIRE(stats.instances == 0);
    REQUIRE(stats.culled_items == 1);
}

TEST_CASE("freed items and stale RIDs stop drawing", "[rendering][canvas]")
{
    CanvasFixture f;
    CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(item, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, red);
    REQUIRE(f.frame().instances == 1);

    f.server.canvasItemFree(item);
    f.server.canvasItemAddRect(item, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, red);
    f.server.canvasItemFree(item);
    REQUIRE(f.frame().instances == 0);

    TextureRid texture = f.server.texture2dCreate({ .size = { 2, 2 } }, {});
    CanvasItemRid sprite = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddTextureRect(sprite, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, texture);
    REQUIRE(f.frame().instances == 1);

    f.server.textureFree(texture);
    REQUIRE(f.frame().instances == 0);
}

TEST_CASE("texture and viewport mirrors answer on the main thread", "[rendering][canvas]")
{
    CanvasFixture f;

    TextureRid texture = f.server.texture2dCreate({ .size = { 3, 5 } }, {});
    REQUIRE(f.server.textureGetSize(texture).x == 3);
    REQUIRE(f.server.textureGetSize(texture).y == 5);
    REQUIRE_FALSE(f.server.texture2dCreate({ .size = { 2, 2 } }, std::vector<std::byte>(3)).isValid());

    TextureRid target = f.server.viewportGetTexture(f.viewport);
    REQUIRE(target.isValid());
    REQUIRE(f.server.textureGetSize(target).x == 100);
    f.server.viewportSetSize(f.viewport, { 40, 30 });
    REQUIRE(f.server.viewportGetSize(f.viewport).y == 30);
    REQUIRE(f.server.textureGetSize(target).x == 40);

    f.server.textureFree(target);
    REQUIRE(f.server.textureGetSize(target).x == 40);

    f.server.viewportFree(f.viewport);
    REQUIRE_FALSE(f.server.viewportGetTexture(f.viewport).isValid());
    REQUIRE(f.server.textureGetSize(target).x == 0);
}

TEST_CASE("texture readback reaches the callback after a frame", "[rendering][canvas]")
{
    for (bool threaded : { false, true }) {
        CAPTURE(threaded);
        CanvasFixture f({ .platform = PlatformKind::Null, .render_thread = threaded });
        REQUIRE(f.server.renderThreadActive() == threaded);

        std::vector<std::byte> pixels(2 * 2 * 4);
        for (std::size_t i = 0; i < pixels.size(); ++i) pixels[i] = static_cast<std::byte>(i);
        TextureRid texture = f.server.texture2dCreate({ .size = { 2, 2 } }, pixels);

        std::optional<TextureData> result;
        f.server.textureRequestData(texture, [&result](TextureData data) { result = std::move(data); });
        REQUIRE_FALSE(result.has_value());

        f.frame();
        REQUIRE(result.has_value());
        REQUIRE(result->size.x == 2);
        REQUIRE(result->pixels == pixels);
    }
}

TEST_CASE("the render thread runs frames one behind the main thread", "[rendering][canvas][thread]")
{
    CanvasFixture f({ .platform = PlatformKind::Null, .render_thread = true });
    REQUIRE(f.server.renderThreadActive());

    std::vector<CanvasItemRid> items;
    for (int frame = 0; frame < 20; ++frame) {
        CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
        f.server.canvasItemAddRect(item, { { static_cast<float>(frame), 0.0f }, { 1.0f, 1.0f } }, red);
        f.server.canvasItemSetTransform(item, Affine2d::Translation({ 1.0f, 1.0f }));
        items.push_back(item);
        if (frame % 3 == 0) {
            f.server.canvasItemFree(items.front());
            items.erase(items.begin());
        }
        f.server.sync();
    }
    f.server.waitIdle();

    const FrameStats stats = f.server.lastFrameStats();
    REQUIRE(stats.frame == 20);
    REQUIRE(stats.instances == items.size());
    REQUIRE(f.server.pendingCommandCount() == 0);
}

TEST_CASE("the render graph runs writers before readers", "[rendering][canvas]")
{
    NullPlatform platform;
    REQUIRE(platform.initialize({}));
    TextureHandle offscreen = platform.textureCreate({ .size = { 4, 4 }, .usage = TextureUsage::RenderTarget | TextureUsage::Sampled });
    TextureHandle screen = platform.textureCreate({ .size = { 4, 4 }, .usage = TextureUsage::RenderTarget });

    RenderGraph graph;
    std::vector<int> order;
    graph.addPass(
        { .name = "screen", .color_attachments = { { .texture = screen } }, .sampled = { offscreen } }, [&order](CommandList &) { order.push_back(2); });
    graph.addPass({ .name = "offscreen", .color_attachments = { { .texture = offscreen } } }, [&order](CommandList &) { order.push_back(1); });

    platform.frameBegin();
    graph.execute(platform.commandListAcquire(QueueType::Graphics));
    platform.frameEnd();

    REQUIRE(order == std::vector<int>{ 1, 2 });

    RenderGraph cycle;
    cycle.addPass({ .name = "a", .color_attachments = { { .texture = offscreen } }, .sampled = { screen } }, [](CommandList &) {});
    cycle.addPass({ .name = "b", .color_attachments = { { .texture = screen } }, .sampled = { offscreen } }, [](CommandList &) {});
    platform.frameBegin();
    REQUIRE_THROWS_AS(cycle.execute(platform.commandListAcquire(QueueType::Graphics)), std::logic_error);
    platform.frameEnd();
}

TEST_CASE("a viewport content rect clips items and fills its area", "[rendering][canvas]")
{
    CanvasFixture f;
    f.server.viewportSetContentRect(f.viewport, Rect2{ { 10.0f, 20.0f }, { 80.0f, 60.0f } });

    CanvasItemRid inside = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(inside, { { 30.0f, 30.0f }, { 10.0f, 10.0f } }, red);
    CanvasItemRid bars = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(bars, { { 0.0f, 0.0f }, { 100.0f, 10.0f } }, red);

    FrameStats stats = f.frame();
    REQUIRE(stats.instances == 2);
    REQUIRE(stats.culled_items == 1);
    REQUIRE(stats.draw_calls == 1);

    f.server.viewportSetContentRect(f.viewport, std::nullopt);
    stats = f.frame();
    REQUIRE(stats.instances == 2);
    REQUIRE(stats.culled_items == 0);
}
