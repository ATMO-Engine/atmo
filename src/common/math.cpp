#include "math.hpp"
#include "project/project_manager.hpp"


namespace atmo
{
    namespace common
    {
        namespace math
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

            float PixelToMeter(float pixels)
            {
                return pixels / atmo::project::ProjectManager::GetSettings().engine.pixels_per_meter;
            }
        } // namespace math
    } // namespace common
} // namespace atmo
