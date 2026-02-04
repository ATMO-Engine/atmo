#include "math.hpp"

namespace atmo
{
    namespace common
    {
        namespace math
        {
            double DegreesToRadians(double degrees)
            {
                return degrees * (PI / 180.0);
            }

            double RadiansToDegrees(double radians)
            {
                return radians * (180.0 / PI);
            }
        } // namespace math
    } // namespace common
} // namespace atmo
