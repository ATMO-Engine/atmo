#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/types.hpp"
#include "server/rendering/canvas_commands.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/rid_allocator.hpp"
#include "server/rendering/text/glyph_atlas.hpp"
#include "server/rendering/types.hpp"

struct TTF_Font;
struct TTF_Text;

namespace atmo::server::rendering
{
    class RenderingServer;

    class FontSystem
    {
    public:
        explicit FontSystem(RenderingServer &server);
        ~FontSystem();

        FontSystem(const FontSystem &) = delete;
        FontSystem &operator=(const FontSystem &) = delete;

        FontRid create(std::span<const std::byte> data, std::string_view name);
        void free(FontRid font);
        [[nodiscard]] bool isAlive(FontRid font) const;

        [[nodiscard]] TTF_Font *face(FontRid font, float size, FontStyle style, float raster_scale);
        [[nodiscard]] core::types::Vector2 measure(FontRid font, std::string_view utf8, float size, FontStyle style, float wrap_width = 0.0f);
        [[nodiscard]] float lineHeight(FontRid font, float size, FontStyle style);

        void appendText(std::vector<CanvasCommand> &out, FontRid font, std::string_view utf8, const TextOptions &options);
        void appendTtfText(std::vector<CanvasCommand> &out, TTF_Text *text, core::types::Vector2 position, const ColorRgba &color, float raster_scale);

        void flush();
        void release();

        [[nodiscard]] const GlyphAtlas &atlas() const noexcept;
        [[nodiscard]] std::size_t cachedRunCount() const noexcept;

    private:
        struct FaceKey {
            int size = 0;
            int dpi = 0;
            FontStyle style;

            bool operator==(const FaceKey &) const noexcept = default;
        };

        struct Face {
            FaceKey key;
            TTF_Font *font = nullptr;
        };

        struct Font {
            std::shared_ptr<const std::vector<std::byte>> data;
            std::string name;
            std::vector<Face> faces;
        };

        struct RunGlyph {
            std::uint32_t page = 0;
            Rect2 rect;
            std::array<float, 4> uv{};
        };

        struct Run {
            std::vector<RunGlyph> glyphs;
            std::vector<Rect2> fills;
            std::uint64_t last_used = 0;
        };

        struct RunKey {
            const TTF_Font *face = nullptr;
            int wrap = 0;
            std::string text;

            bool operator==(const RunKey &) const noexcept = default;
        };

        struct RunKeyHash {
            std::size_t operator()(const RunKey &key) const noexcept;
        };

        bool layout(TTF_Text *text, Run &run);
        void emit(std::vector<CanvasCommand> &out, const Run &run, core::types::Vector2 position, const ColorRgba &color, float raster_scale) const;
        void closeFaces(Font &font);

        RenderingServer &m_server;
        GlyphAtlas m_atlas;
        RidAllocator<RidKind::Font> m_rids;
        std::unordered_map<std::uint32_t, Font> m_fonts;
        std::unordered_map<RunKey, Run, RunKeyHash> m_runs;
        std::uint64_t m_frame = 0;
        bool m_ttf = false;
    };
} // namespace atmo::server::rendering
