#pragma once

namespace atmo
{
    namespace core
    {
        namespace types
        {
            typedef struct vector2 {
                float x = 0.0f;
                float y = 0.0f;
            } vector2;

            typedef struct vector2i {
                int x = 0;
                int y = 0;
            } vector2i;

            typedef struct vector3 {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
            } vector3;

            typedef struct vector3i {
                int x = 0;
                int y = 0;
                int z = 0;
            } vector3i;
        } // namespace types
    } // namespace core
} // namespace atmo
