#include <cstddef>
#include <fstream>
#include <iterator>
#include <optional>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "server/rendering/rendering_server.hpp"
#include "server/rendering/text/font_system.hpp"
#include "server/rendering/text/glyph_atlas.hpp"

using namespace atmo::server::rendering;
using atmo::core::types::Color;
using atmo::core::types::Vector2;
using atmo::core::types::Vector2i;

namespace
{
    constexpr const char *font_path = "assets/fonts/Nunito/Nunito.ttf";

    std::vector<std::byte> ReadFile(const char *path)
    {
        std::ifstream file(path, std::ios::binary);
        std::vector<char> chars{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        std::vector<std::byte> bytes(chars.size());
        for (std::size_t i = 0; i < chars.size(); ++i) bytes[i] = static_cast<std::byte>(chars[i]);
        return bytes;
    }

    struct TextFixture {
        TextFixture()
        {
            REQUIRE(server.init({ .platform = PlatformKind::Null, .render_thread = false }));
            const std::vector<std::byte> data = ReadFile(font_path);
            REQUIRE_FALSE(data.empty());
            font = server.fontCreate(data, font_path);
            REQUIRE(font.isValid());
            canvas = server.canvasCreate();
            viewport = server.viewportCreate();
            server.viewportSetSize(viewport, { 256, 128 });
            server.viewportSetCanvas(viewport, canvas);
        }

        ~TextFixture()
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
        FontRid font;
        CanvasRid canvas;
        ViewportRid viewport;
    };
} // namespace

TEST_CASE("the shelf packer fills shelves before opening new ones", "[rendering][text]")
{
    ShelfPacker packer({ 64, 64 });

    const std::optional<Vector2i> first = packer.insert({ 30, 10 });
    const std::optional<Vector2i> second = packer.insert({ 30, 10 });
    const std::optional<Vector2i> third = packer.insert({ 30, 10 });
    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(third);
    REQUIRE(first->y == 0);
    REQUIRE(second->x == 30);
    REQUIRE(second->y == 0);
    REQUIRE(third->x == 0);
    REQUIRE(third->y == 10);

    REQUIRE_FALSE(packer.insert({ 65, 1 }));
    REQUIRE_FALSE(packer.insert({ 0, 4 }));
    REQUIRE(packer.insert({ 64, 44 }));
    REQUIRE_FALSE(packer.insert({ 64, 1 }));
}

TEST_CASE("fonts measure text in logical units", "[rendering][text]")
{
    TextFixture f;

    const Vector2 short_text = f.server.fontMeasureText(f.font, "Atmo", 16.0f);
    const Vector2 long_text = f.server.fontMeasureText(f.font, "Atmo engine", 16.0f);
    const Vector2 large_text = f.server.fontMeasureText(f.font, "Atmo", 32.0f);
    REQUIRE(short_text.x > 0.0f);
    REQUIRE(short_text.y > 0.0f);
    REQUIRE(long_text.x > short_text.x);
    REQUIRE(large_text.x > short_text.x * 1.5f);
    REQUIRE(f.server.fontGetLineHeight(f.font, 16.0f) >= short_text.y * 0.9f);

    const Vector2 wrapped = f.server.fontMeasureText(f.font, "Atmo engine Atmo engine", 16.0f, {}, long_text.x + 1.0f);
    REQUIRE(wrapped.y > short_text.y * 1.5f);

    REQUIRE(f.server.fontGetTtfFont(f.font, 16.0f, {}, 1.0f) != f.server.fontGetTtfFont(f.font, 16.0f, {}, 2.0f));
    REQUIRE(f.server.fontGetTtfFont(f.font, 16.0f, {}, 2.0f) == f.server.fontGetTtfFont(f.font, 16.0f, {}, 2.0f));
    REQUIRE(f.server.fontGetTtfFont(f.font, 16.0f, { .bold = true }) != f.server.fontGetTtfFont(f.font, 16.0f));
}

TEST_CASE("invalid font data is rejected", "[rendering][text]")
{
    TextFixture f;
    const std::vector<std::byte> garbage(64, std::byte{ 7 });
    REQUIRE_FALSE(f.server.fontCreate(garbage, "garbage").isValid());
    REQUIRE_FALSE(f.server.fontCreate({}, "empty").isValid());
}

TEST_CASE("text becomes glyph quads from a shared atlas", "[rendering][text]")
{
    TextFixture f;
    const GlyphAtlas &atlas = f.server.fonts().atlas();

    CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddText(item, f.font, "AB AB", { .position = { 4.0f, 4.0f }, .size = 16.0f });
    FrameStats stats = f.frame();
    REQUIRE(stats.instances == 4);
    REQUIRE(stats.draw_calls == 1);
    REQUIRE(atlas.pageCount() == 1);
    const std::size_t glyphs = atlas.glyphCount();
    REQUIRE(glyphs >= 2);

    f.server.canvasItemClear(item);
    f.server.canvasItemAddText(item, f.font, "BABA", { .position = { 4.0f, 40.0f }, .size = 16.0f });
    stats = f.frame();
    REQUIRE(stats.instances == 4);
    REQUIRE(atlas.glyphCount() == glyphs);

    f.server.canvasItemClear(item);
    f.server.canvasItemAddText(item, f.font, "AB", { .position = { 4.0f, 40.0f }, .size = 16.0f, .raster_scale = 2.0f });
    stats = f.frame();
    REQUIRE(stats.instances == 2);
    REQUIRE(atlas.glyphCount() > glyphs);
}

TEST_CASE("text, rects and textures share one draw", "[rendering][text]")
{
    TextFixture f;
    std::vector<std::byte> pixels(4 * 4 * 4, std::byte{ 0xFF });
    const TextureRid texture = f.server.texture2dCreate({ .size = { 4, 4 } }, pixels);

    CanvasCommandWriter writer;
    writer.addRect({ { 0.0f, 0.0f }, { 200.0f, 40.0f } }, Color(0.2f, 0.2f, 0.2f, 1.0f), { .corner_radii = { 6.0f, 6.0f, 6.0f, 6.0f } });
    writer.addText(f.font, "Hi", { .position = { 8.0f, 8.0f }, .size = 14.0f });
    writer.addTextureRect({ { 100.0f, 8.0f }, { 16.0f, 16.0f } }, texture);
    writer.addRect(
        { { 0.0f, 0.0f }, { 200.0f, 40.0f } },
        Color(0.0f, 0.0f, 0.0f, 0.0f),
        { .border_widths = { 1.0f, 1.0f, 1.0f, 1.0f }, .border_color = { 1.0f, 1.0f, 1.0f, 1.0f } });
    REQUIRE(writer.size() == 4);

    const CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemSetCommands(item, std::move(writer));
    REQUIRE(writer.empty());

    const FrameStats stats = f.frame();
    REQUIRE(stats.instances == 5);
    REQUIRE(stats.draw_calls == 1);
}

TEST_CASE("setting commands replaces an item's content", "[rendering][canvas]")
{
    TextFixture f;
    const CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(item, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, Color(1.0f, 0.0f, 0.0f, 1.0f));
    f.server.canvasItemAddRect(item, { { 20.0f, 0.0f }, { 10.0f, 10.0f } }, Color(1.0f, 0.0f, 0.0f, 1.0f));
    REQUIRE(f.frame().instances == 2);

    CanvasCommandWriter writer;
    writer.addRect({ { 0.0f, 0.0f }, { 10.0f, 10.0f } }, Color(0.0f, 1.0f, 0.0f, 1.0f));
    f.server.canvasItemSetCommands(item, std::move(writer));
    REQUIRE(f.frame().instances == 1);

    CanvasCommandWriter more;
    more.addRect({ { 40.0f, 0.0f }, { 10.0f, 10.0f } }, Color(0.0f, 1.0f, 0.0f, 1.0f));
    f.server.canvasItemAddCommands(item, std::move(more));
    REQUIRE(f.frame().instances == 2);
}

TEST_CASE("a command transform rotates the commands after it", "[rendering][canvas]")
{
    TextFixture f;
    const CanvasItemRid item = f.server.canvasItemCreate(f.canvas);

    CanvasCommandWriter writer;
    writer.setTransform(Affine2d::Translation({ 1000.0f, 0.0f }));
    writer.addRect({ { 0.0f, 0.0f }, { 10.0f, 10.0f } }, Color(1.0f, 0.0f, 0.0f, 1.0f));
    writer.setTransform(Affine2d::Identity());
    writer.addRect({ { 0.0f, 0.0f }, { 10.0f, 10.0f } }, Color(1.0f, 0.0f, 0.0f, 1.0f));
    f.server.canvasItemSetCommands(item, std::move(writer));

    const FrameStats stats = f.frame();
    REQUIRE(stats.instances == 1);
    REQUIRE(stats.culled_items == 1);
}

TEST_CASE("canvas layers draw with the viewport but not its canvas transform", "[rendering][canvas]")
{
    TextFixture f;
    const CanvasItemRid world = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddRect(world, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, Color(1.0f, 0.0f, 0.0f, 1.0f));
    f.server.viewportSetCanvasTransform(f.viewport, Affine2d::Translation({ 5000.0f, 0.0f }));

    const CanvasRid overlay = f.server.canvasCreate();
    const CanvasItemRid ui = f.server.canvasItemCreate(overlay);
    f.server.canvasItemAddRect(ui, { { 0.0f, 0.0f }, { 10.0f, 10.0f } }, Color(0.0f, 1.0f, 0.0f, 1.0f));
    f.server.viewportAttachCanvas(f.viewport, overlay, 1);

    FrameStats stats = f.frame();
    REQUIRE(stats.canvas_items == 2);
    REQUIRE(stats.instances == 1);
    REQUIRE(stats.culled_items == 1);

    f.server.viewportDetachCanvas(f.viewport, overlay);
    stats = f.frame();
    REQUIRE(stats.canvas_items == 1);
    REQUIRE(stats.instances == 0);
}

TEST_CASE("freeing a font forgets its glyphs and cached runs", "[rendering][text]")
{
    TextFixture f;
    const CanvasItemRid item = f.server.canvasItemCreate(f.canvas);
    f.server.canvasItemAddText(item, f.font, "Glyphs", { .size = 16.0f });
    f.frame();
    REQUIRE(f.server.fonts().atlas().glyphCount() > 0);
    REQUIRE(f.server.fonts().cachedRunCount() == 1);

    f.server.fontFree(f.font);
    REQUIRE_FALSE(f.server.fontIsAlive(f.font));
    REQUIRE(f.server.fonts().atlas().glyphCount() == 0);
    REQUIRE(f.server.fonts().cachedRunCount() == 0);
    REQUIRE(f.server.fontMeasureText(f.font, "Glyphs", 16.0f).x == 0.0f);
}
