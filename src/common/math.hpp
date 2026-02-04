#pragma once

namespace atmo
{
    namespace common
    {
        namespace math
        {
            constexpr double PI = 3.14159265358979323846f;

            /**
             * @brief Convert degrees to radians
             *
             * @param degrees Angle in degrees
             * @return double Angle in radians
             */
            double DegreesToRadians(double degrees);

            /**
             * @brief Convert radians to degrees
             *
             * @param radians Angle in radians
             * @return double Angle in degrees
             */
            double RadiansToDegrees(double radians);

            /**
             * @brief Clamp a value between a minimum and maximum
             *
             * @tparam T Type of the value
             * @param value Value to clamp
             * @param min Minimum value
             * @param max Maximum value
             * @return T Clamped value
             */
            template <typename T> T Clamp(T value, T min, T max)
            {
                if (value < min)
                    return min;
                if (value > max)
                    return max;
                return value;
            }

            /**
             * @brief Linearly interpolate between two values
             *
             * @tparam T Type of the values
             * @param a Start value
             * @param b End value
             * @param t Interpolation factor (0.0 to 1.0)
             * @return T Interpolated value
             */
            template <typename T> T Lerp(T a, T b, float t)
            {
                return a + static_cast<T>(t * (b - a));
            }
        } // namespace math
    } // namespace common
} // namespace atmo
