#pragma once

#include <clay.h>

#include "server/rendering/canvas_commands.hpp"

namespace atmo::impl
{
    Clay_Dimensions ClayMeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *user_data);

    void ClayRenderCommands(const Clay_RenderCommandArray &commands, server::rendering::CanvasCommandWriter &writer, float raster_scale);
} // namespace atmo::impl
