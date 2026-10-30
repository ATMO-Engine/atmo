#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "core/types.hpp"
#include "server/rendering/platform/rendering_platform.hpp"
#include "server/rendering/rid.hpp"

namespace atmo::server::rendering
{
    class RenderingServer;

    class ShelfPacker
    {
    public:
        explicit ShelfPacker(core::types::Vector2i size) noexcept;

        [[nodiscard]] std::optional<core::types::Vector2i> insert(core::types::Vector2i size);
        [[nodiscard]] core::types::Vector2i size() const noexcept;

    private:
        struct Shelf {
            int y = 0;
            int height = 0;
            int cursor = 0;
        };

        core::types::Vector2i m_size;
        std::vector<Shelf> m_shelves;
        int m_next_y = 0;
    };

    class GlyphAtlas
    {
    public:
        struct Glyph {
            std::uint32_t page = 0;
            Region2i region;
            bool blank = false;
        };

        explicit GlyphAtlas(RenderingServer &server, int page_size = 1024);
        ~GlyphAtlas();

        GlyphAtlas(const GlyphAtlas &) = delete;
        GlyphAtlas &operator=(const GlyphAtlas &) = delete;

        [[nodiscard]] const Glyph *find(const void *face, std::uint32_t glyph) const;
        const Glyph *insert(const void *face, std::uint32_t glyph, core::types::Vector2i size, std::span<const std::byte> rgba);
        void forget(const void *face);
        void flush();
        void release();

        [[nodiscard]] TextureRid pageTexture(std::uint32_t page) const;
        [[nodiscard]] int pageSize() const noexcept;
        [[nodiscard]] std::size_t pageCount() const noexcept;
        [[nodiscard]] std::size_t glyphCount() const noexcept;

    private:
        struct Key {
            const void *face = nullptr;
            std::uint32_t glyph = 0;

            bool operator==(const Key &) const noexcept = default;
        };

        struct KeyHash {
            std::size_t operator()(const Key &key) const noexcept;
        };

        struct Page {
            TextureRid texture;
            ShelfPacker packer;
            std::vector<std::byte> pixels;
            std::optional<Region2i> dirty;
        };

        Page *addPage();

        RenderingServer &m_server;
        int m_page_size;
        std::vector<Page> m_pages;
        std::unordered_map<Key, Glyph, KeyHash> m_glyphs;
    };
} // namespace atmo::server::rendering
