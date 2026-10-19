#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>

#include "core/types.hpp"
#include "server/rendering/platform/rendering_platform.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/types.hpp"

namespace atmo::server::rendering
{
    struct CanvasRectCommand {
        Rect2 rect;
        ColorRgba color = white;
    };

    struct CanvasTextureRectCommand {
        Rect2 rect;
        TextureRid texture;
        TextureRectOptions options;
    };

    struct CanvasPolygonCommand {
        std::vector<core::types::Vector2> points;
        std::vector<ColorRgba> colors;
    };

    struct CanvasPolylineCommand {
        std::vector<core::types::Vector2> points;
        ColorRgba color = white;
        float width = 1.0f;
        bool closed = false;
    };

    struct CanvasClipPushCommand {
        Rect2 rect;
    };

    struct CanvasClipPopCommand {
    };

    using CanvasCommand =
        std::variant<CanvasRectCommand, CanvasTextureRectCommand, CanvasPolygonCommand, CanvasPolylineCommand, CanvasClipPushCommand, CanvasClipPopCommand>;

    struct CanvasItemData {
        CanvasRid canvas;
        CanvasItemRid parent;
        Affine2d transform;
        ColorRgba modulate = white;
        int z_index = 0;
        bool visible = true;
        std::uint64_t order = 0;
        std::size_t slot = 0;
        std::vector<CanvasCommand> commands;

        std::uint64_t cache_frame = 0;
        Affine2d global;
        ColorRgba global_modulate = white;
        bool global_visible = true;
    };

    struct CanvasData {
        std::vector<CanvasItemRid> items;
        std::vector<CanvasItemRid> sorted;
        bool sort_dirty = true;
    };

    struct TextureEntry {
        TextureHandle handle;
        core::types::Vector2i size{};
    };

    struct CanvasStorage {
        std::unordered_map<std::uint32_t, CanvasData> canvases;
        std::unordered_map<std::uint32_t, CanvasItemData> items;
        std::unordered_map<std::uint32_t, TextureEntry> textures;
        std::uint64_t next_order = 1;
    };
} // namespace atmo::server::rendering
