#pragma once

#include <any>
#include <map>
#include <vector>

#include "box2d/box2d.h"
#include "flecs.h"
#include "glaze/glaze.hpp"
#include "project/file_system.hpp"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            class Scene
            {
            public:
                Scene(flecs::world &world, flecs::entity root);
                ~Scene() = default;

                /**
                 * @brief Initialize the scene's 2D and 3D worlds
                 *
                 */
                void init();

                /**
                 * @brief Destroy the 2D and 3D worlds cleanly
                 *
                 */
                void destroy();

                /**
                 * @brief Load a scene from a file
                 *
                 * @param world The ECS world to load the scene into
                 * @param file The file to load the scene from
                 * @return Scene* The loaded scene (uninitialized)
                 */
                static Scene *LoadFromFile(flecs::world &world, project::File &file);

            private:
                // flecs::entity m_scope;
                flecs::world &m_world;
                b2WorldId m_box2d_world;
            };
        } // namespace scene
    } // namespace core
} // namespace atmo
