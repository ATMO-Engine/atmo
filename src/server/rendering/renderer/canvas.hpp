#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "core/types.hpp"
#include "server/rendering/canvas_commands.hpp"
#include "server/rendering/platform/rendering_platform.hpp"
#include "server/rendering/rid.hpp"
#include "server/rendering/types.hpp"

namespace atmo::server::rendering
{
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
