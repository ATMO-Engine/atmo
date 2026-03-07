#include "math.hpp"
#include "project/project_manager.hpp"


namespace atmo::common::math
{
    float DegreesToRadians(float degrees)
    {
        return degrees * (PI / 180.0);
    }

    float RadiansToDegrees(float radians)
    {
        return radians * (180.0 / PI);
    }

    float MeterToPixel(float meters)
    {
        return meters * atmo::project::ProjectManager::GetSettings().engine.pixels_per_meter;
    }

    core::types::Vector2 MeterToPixel(const core::types::Vector2 &meters)
    {
        return { MeterToPixel(meters.x), MeterToPixel(meters.y) };
    }

    float PixelToMeter(float pixels)
    {
        return pixels / atmo::project::ProjectManager::GetSettings().engine.pixels_per_meter;
    }

    core::types::Vector2 PixelToMeter(const core::types::Vector2 &pixels)
    {
        return { PixelToMeter(pixels.x), PixelToMeter(pixels.y) };
    }
} // namespace atmo::common::math
