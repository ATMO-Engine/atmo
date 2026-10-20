#include "glyph_atlas.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <functional>

#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
    namespace
    {
        constexpr int padding = 1;

        Region2i Union(const Region2i &a, const Region2i &b) noexcept
        {
            const int left = std::min(a.x, b.x);
            const int top = std::min(a.y, b.y);
            const int right = std::max(a.x + a.width, b.x + b.width);
            const int bottom = std::max(a.y + a.height, b.y + b.height);
            return { .x = left, .y = top, .width = right - left, .height = bottom - top };
        }
    } // namespace

    ShelfPacker::ShelfPacker(core::types::Vector2i size) noexcept : m_size(size) {}

    std::optional<core::types::Vector2i> ShelfPacker::insert(core::types::Vector2i size)
    {
        if (size.x <= 0 || size.y <= 0 || size.x > m_size.x || size.y > m_size.y)
            return std::nullopt;

        Shelf *best = nullptr;
        for (Shelf &shelf : m_shelves) {
            if (shelf.height < size.y || shelf.cursor + size.x > m_size.x)
                continue;
            if (!best || shelf.height < best->height)
                best = &shelf;
        }

        if (!best || best->height > size.y + (size.y / 2) + 2) {
            if (m_next_y + size.y <= m_size.y) {
                m_shelves.push_back({ .y = m_next_y, .height = size.y });
                m_next_y += size.y;
                best = &m_shelves.back();
            } else if (!best) {
                return std::nullopt;
            }
        }

        const core::types::Vector2i position{ best->cursor, best->y };
        best->cursor += size.x;
        return position;
    }

    core::types::Vector2i ShelfPacker::size() const noexcept
    {
        return m_size;
    }

    std::size_t GlyphAtlas::KeyHash::operator()(const Key &key) const noexcept
    {
        return std::hash<const void *>{}(key.face) ^ (std::hash<std::uint32_t>{}(key.glyph) * 0x9E3779B97F4A7C15ULL);
    }

    GlyphAtlas::GlyphAtlas(RenderingServer &server, int page_size) : m_server(server), m_page_size(page_size) {}

    GlyphAtlas::~GlyphAtlas()
    {
        release();
    }

    const GlyphAtlas::Glyph *GlyphAtlas::find(const void *face, std::uint32_t glyph) const
    {
        auto it = m_glyphs.find({ face, glyph });
        return it == m_glyphs.end() ? nullptr : &it->second;
    }

    GlyphAtlas::Page *GlyphAtlas::addPage()
    {
        const std::size_t bytes = static_cast<std::size_t>(m_page_size) * static_cast<std::size_t>(m_page_size) * 4;
        Page page{ .packer = ShelfPacker({ m_page_size, m_page_size }), .pixels = std::vector<std::byte>(bytes, std::byte{ 0xFF }) };
        for (std::size_t alpha = 3; alpha < bytes; alpha += 4) page.pixels[alpha] = std::byte{ 0 };
        page.texture =
            m_server.texture2dCreate({ .size = { m_page_size, m_page_size }, .debug_name = std::format("glyph atlas {}", m_pages.size()) }, page.pixels);
        if (!page.texture.isValid())
            return nullptr;
        m_pages.push_back(std::move(page));
        return &m_pages.back();
    }

    const GlyphAtlas::Glyph *GlyphAtlas::insert(const void *face, std::uint32_t glyph, core::types::Vector2i size, std::span<const std::byte> rgba)
    {
        if (const Glyph *existing = find(face, glyph))
            return existing;

        if (size.x <= 0 || size.y <= 0 || rgba.size() < static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) * 4)
            return &m_glyphs.emplace(Key{ face, glyph }, Glyph{ .blank = true }).first->second;

        const core::types::Vector2i padded{ size.x + (padding * 2), size.y + (padding * 2) };
        if (padded.x > m_page_size || padded.y > m_page_size) {
            spdlog::warn("Glyph {} is {}x{}, larger than a {}px atlas page", glyph, size.x, size.y, m_page_size);
            return nullptr;
        }

        Page *page = nullptr;
        std::optional<core::types::Vector2i> position;
        if (!m_pages.empty()) {
            page = &m_pages.back();
            position = page->packer.insert(padded);
        }
        if (!position) {
            page = addPage();
            if (!page)
                return nullptr;
            position = page->packer.insert(padded);
            if (!position)
                return nullptr;
        }

        const Region2i region{ .x = position->x + padding, .y = position->y + padding, .width = size.x, .height = size.y };
        const std::size_t row_bytes = static_cast<std::size_t>(size.x) * 4;
        for (int y = 0; y < size.y; ++y) {
            const std::size_t offset =
                ((static_cast<std::size_t>(region.y + y) * static_cast<std::size_t>(m_page_size)) + static_cast<std::size_t>(region.x)) * 4;
            std::memcpy(page->pixels.data() + offset, rgba.data() + (static_cast<std::size_t>(y) * row_bytes), row_bytes);
        }
        page->dirty = page->dirty ? Union(*page->dirty, region) : region;

        const auto page_index = static_cast<std::uint32_t>(page - m_pages.data());
        return &m_glyphs.emplace(Key{ face, glyph }, Glyph{ .page = page_index, .region = region }).first->second;
    }

    void GlyphAtlas::forget(const void *face)
    {
        std::erase_if(m_glyphs, [face](const auto &entry) { return entry.first.face == face; });
    }

    void GlyphAtlas::flush()
    {
        for (Page &page : m_pages) {
            if (!page.dirty)
                continue;
            const Region2i region = *page.dirty;
            page.dirty.reset();

            const std::size_t row_bytes = static_cast<std::size_t>(region.width) * 4;
            std::vector<std::byte> pixels(row_bytes * static_cast<std::size_t>(region.height));
            for (int y = 0; y < region.height; ++y) {
                const std::size_t offset =
                    ((static_cast<std::size_t>(region.y + y) * static_cast<std::size_t>(m_page_size)) + static_cast<std::size_t>(region.x)) * 4;
                std::memcpy(pixels.data() + (static_cast<std::size_t>(y) * row_bytes), page.pixels.data() + offset, row_bytes);
            }
            m_server.texture2dUpdate(page.texture, region, pixels);
        }
    }

    void GlyphAtlas::release()
    {
        for (Page &page : m_pages) m_server.textureFree(page.texture);
        m_pages.clear();
        m_glyphs.clear();
    }

    TextureRid GlyphAtlas::pageTexture(std::uint32_t page) const
    {
        return page < m_pages.size() ? m_pages[page].texture : TextureRid{};
    }

    int GlyphAtlas::pageSize() const noexcept
    {
        return m_page_size;
    }

    std::size_t GlyphAtlas::pageCount() const noexcept
    {
        return m_pages.size();
    }

    std::size_t GlyphAtlas::glyphCount() const noexcept
    {
        return m_glyphs.size();
    }
} // namespace atmo::server::rendering
