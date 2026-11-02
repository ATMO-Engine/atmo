#include "font_system.hpp"

#include <cmath>
#include <cstring>
#include <functional>

#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_surface.h"
#include "SDL3_ttf/SDL_textengine.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
    namespace
    {
        constexpr std::uint64_t run_lifetime_frames = 120;
        constexpr float base_dpi = 96.0f;

        int Pixels(float value, float scale) noexcept
        {
            return static_cast<int>(std::lround(value * scale));
        }
    } // namespace

    std::size_t FontSystem::RunKeyHash::operator()(const RunKey &key) const noexcept
    {
        std::size_t hash = std::hash<std::string>{}(key.text);
        hash ^= std::hash<const void *>{}(key.face) + 0x9E3779B97F4A7C15ULL + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int>{}(key.wrap) + 0x9E3779B97F4A7C15ULL + (hash << 6) + (hash >> 2);
        return hash;
    }

    FontSystem::FontSystem(RenderingServer &server) : m_server(server), m_atlas(server)
    {
        m_ttf = TTF_Init();
        if (!m_ttf)
            spdlog::error("Failed to initialize SDL_ttf: {}", SDL_GetError());
    }

    FontSystem::~FontSystem()
    {
        release();
        if (m_ttf)
            TTF_Quit();
    }

    FontRid FontSystem::create(std::span<const std::byte> data, std::string_view name)
    {
        if (!m_ttf || data.empty())
            return {};

        auto bytes = std::make_shared<const std::vector<std::byte>>(data.begin(), data.end());
        SDL_IOStream *io = SDL_IOFromConstMem(bytes->data(), bytes->size());
        TTF_Font *probe = io ? TTF_OpenFontIO(io, true, 12.0f) : nullptr;
        if (!probe) {
            spdlog::error("fontCreate: {} is not a font SDL_ttf can open: {}", name, SDL_GetError());
            return {};
        }
        TTF_CloseFont(probe);

        const FontRid font = m_rids.allocate();
        m_fonts[font.raw()] = Font{ .data = std::move(bytes), .name = std::string(name) };
        return font;
    }

    void FontSystem::closeFaces(Font &font)
    {
        for (Face &face : font.faces) {
            m_atlas.forget(face.font);
            std::erase_if(m_runs, [&face](const auto &entry) { return entry.first.face == face.font; });
            TTF_CloseFont(face.font);
        }
        font.faces.clear();
    }

    void FontSystem::free(FontRid font)
    {
        if (!m_rids.release(font))
            return;
        auto it = m_fonts.find(font.raw());
        if (it == m_fonts.end())
            return;
        closeFaces(it->second);
        m_fonts.erase(it);
    }

    bool FontSystem::isAlive(FontRid font) const
    {
        return m_rids.isAlive(font);
    }

    TTF_Font *FontSystem::face(FontRid font, float size, FontStyle style, float raster_scale)
    {
        if (!m_rids.isAlive(font) || size <= 0.0f || raster_scale <= 0.0f)
            return nullptr;
        auto it = m_fonts.find(font.raw());
        if (it == m_fonts.end())
            return nullptr;
        Font &data = it->second;

        const FaceKey key{ .size = Pixels(size, 64.0f), .dpi = Pixels(base_dpi, raster_scale), .style = style };
        for (const Face &face : data.faces)
            if (face.key == key)
                return face.font;

        SDL_IOStream *io = SDL_IOFromConstMem(data.data->data(), data.data->size());
        TTF_Font *opened = io ? TTF_OpenFontIO(io, true, size) : nullptr;
        if (!opened) {
            spdlog::error("Failed to open {} at size {}: {}", data.name, size, SDL_GetError());
            return nullptr;
        }
        if (!TTF_SetFontSizeDPI(opened, size, key.dpi, key.dpi))
            spdlog::error("Failed to size {} at {}pt and {} dpi: {}", data.name, size, key.dpi, SDL_GetError());
        TTF_SetFontStyle(opened, (style.bold ? TTF_STYLE_BOLD : 0) | (style.italic ? TTF_STYLE_ITALIC : 0));

        data.faces.push_back({ .key = key, .font = opened });
        return opened;
    }

    core::types::Vector2 FontSystem::measure(FontRid font, std::string_view utf8, float size, FontStyle style, float wrap_width)
    {
        TTF_Font *ttf = face(font, size, style, 1.0f);
        if (!ttf)
            return { 0.0f, 0.0f };
        if (utf8.empty())
            return { 0.0f, static_cast<float>(TTF_GetFontHeight(ttf)) };

        int width = 0;
        int height = 0;
        const bool measured = wrap_width > 0.0f ? TTF_GetStringSizeWrapped(ttf, utf8.data(), utf8.size(), Pixels(wrap_width, 1.0f), &width, &height)
                                                : TTF_GetStringSize(ttf, utf8.data(), utf8.size(), &width, &height);
        if (!measured)
            spdlog::error("Failed to measure text: {}", SDL_GetError());
        return { static_cast<float>(width), static_cast<float>(height) };
    }

    float FontSystem::lineHeight(FontRid font, float size, FontStyle style)
    {
        TTF_Font *ttf = face(font, size, style, 1.0f);
        return ttf ? static_cast<float>(TTF_GetFontLineSkip(ttf)) : 0.0f;
    }

    bool FontSystem::layout(TTF_Text *text, Run &run)
    {
        if (!text || !TTF_UpdateText(text))
            return false;

        const TTF_TextData *data = text->internal;
        const auto page_size = static_cast<float>(m_atlas.pageSize());
        std::vector<std::byte> pixels;

        for (int i = 0; i < data->num_ops; ++i) {
            const TTF_DrawOperation &op = data->ops[i];
            if (op.cmd == TTF_DRAW_COMMAND_FILL) {
                const SDL_Rect &rect = op.fill.rect;
                run.fills.push_back({ { static_cast<float>(rect.x), static_cast<float>(rect.y) }, { static_cast<float>(rect.w), static_cast<float>(rect.h) } });
                continue;
            }
            if (op.cmd != TTF_DRAW_COMMAND_COPY)
                continue;

            const TTF_CopyOperation &copy = op.copy;
            const GlyphAtlas::Glyph *glyph = m_atlas.find(copy.glyph_font, copy.glyph_index);
            if (!glyph) {
                SDL_Surface *image = TTF_GetGlyphImageForIndex(copy.glyph_font, copy.glyph_index, nullptr);
                SDL_Surface *rgba = image ? SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32) : nullptr;
                if (image)
                    SDL_DestroySurface(image);
                if (!rgba)
                    continue;

                const std::size_t row = static_cast<std::size_t>(rgba->w) * 4;
                pixels.resize(row * static_cast<std::size_t>(rgba->h));
                for (int y = 0; y < rgba->h; ++y)
                    std::memcpy(
                        pixels.data() + (static_cast<std::size_t>(y) * row),
                        static_cast<const std::byte *>(rgba->pixels) + (static_cast<std::size_t>(y) * static_cast<std::size_t>(rgba->pitch)),
                        row);
                glyph = m_atlas.insert(copy.glyph_font, copy.glyph_index, { rgba->w, rgba->h }, pixels);
                SDL_DestroySurface(rgba);
            }
            if (!glyph || glyph->blank || copy.dst.w <= 0 || copy.dst.h <= 0)
                continue;

            const float u0 = static_cast<float>(glyph->region.x + copy.src.x);
            const float v0 = static_cast<float>(glyph->region.y + copy.src.y);
            run.glyphs.push_back(
                {
                    .page = glyph->page,
                    .rect = { { static_cast<float>(copy.dst.x), static_cast<float>(copy.dst.y) },
                              { static_cast<float>(copy.dst.w), static_cast<float>(copy.dst.h) } },
                    .uv = { u0 / page_size,
                            v0 / page_size,
                            (u0 + static_cast<float>(copy.src.w)) / page_size,
                            (v0 + static_cast<float>(copy.src.h)) / page_size },
                });
        }
        return true;
    }

    void FontSystem::emit(std::vector<CanvasCommand> &out, const Run &run, core::types::Vector2 position, const ColorRgba &color, float raster_scale) const
    {
        const float scale = raster_scale > 0.0f ? raster_scale : 1.0f;
        const float origin_x = std::round(position.x * scale);
        const float origin_y = std::round(position.y * scale);
        const auto to_units = [&](const Rect2 &pixels) {
            return Rect2{ { (origin_x + pixels.position.x) / scale, (origin_y + pixels.position.y) / scale },
                          { pixels.size.x / scale, pixels.size.y / scale } };
        };

        CanvasGlyphRunCommand *current = nullptr;
        std::uint32_t current_page = 0;
        for (const RunGlyph &glyph : run.glyphs) {
            if (!current || glyph.page != current_page) {
                out.emplace_back(CanvasGlyphRunCommand{ .texture = m_atlas.pageTexture(glyph.page), .color = color });
                current = &std::get<CanvasGlyphRunCommand>(out.back());
                current_page = glyph.page;
            }
            current->quads.push_back({ .rect = to_units(glyph.rect), .uv = glyph.uv });
        }

        for (const Rect2 &fill : run.fills) out.emplace_back(CanvasRectCommand{ .rect = to_units(fill), .color = color });
    }

    void FontSystem::appendText(std::vector<CanvasCommand> &out, FontRid font, std::string_view utf8, const TextOptions &options)
    {
        if (utf8.empty())
            return;
        TTF_Font *ttf = face(font, options.size, options.style, options.raster_scale);
        if (!ttf)
            return;

        RunKey key{ .face = ttf, .wrap = options.wrap_width > 0.0f ? Pixels(options.wrap_width, options.raster_scale) : 0, .text = std::string(utf8) };
        auto it = m_runs.find(key);
        if (it == m_runs.end()) {
            Run run;
            TTF_Text *text = TTF_CreateText(nullptr, ttf, utf8.data(), utf8.size());
            if (!text) {
                spdlog::error("Failed to lay out text: {}", SDL_GetError());
                return;
            }
            if (key.wrap > 0)
                TTF_SetTextWrapWidth(text, key.wrap);
            const bool laid_out = layout(text, run);
            TTF_DestroyText(text);
            if (!laid_out)
                return;
            it = m_runs.emplace(std::move(key), std::move(run)).first;
        }

        it->second.last_used = m_frame;
        emit(out, it->second, options.position, options.color, options.raster_scale);
    }

    void FontSystem::appendTtfText(std::vector<CanvasCommand> &out, TTF_Text *text, core::types::Vector2 position, const ColorRgba &color, float raster_scale)
    {
        Run run;
        if (layout(text, run))
            emit(out, run, position, color, raster_scale);
    }

    void FontSystem::flush()
    {
        m_atlas.flush();
        ++m_frame;
        if (m_frame > run_lifetime_frames)
            std::erase_if(m_runs, [this](const auto &entry) { return entry.second.last_used + run_lifetime_frames < m_frame; });
    }

    void FontSystem::release()
    {
        m_runs.clear();
        for (auto &[raw, font] : m_fonts) closeFaces(font);
        m_fonts.clear();
        m_rids = {};
        m_atlas.release();
    }

    const GlyphAtlas &FontSystem::atlas() const noexcept
    {
        return m_atlas;
    }

    std::size_t FontSystem::cachedRunCount() const noexcept
    {
        return m_runs.size();
    }
} // namespace atmo::server::rendering
