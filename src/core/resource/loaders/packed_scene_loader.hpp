#pragma once

#include <memory>
#include "core/ecs/entities/entity.hpp"
#include "core/resource/resource.hpp"
#include "flecs.h"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            /**
             * @brief A parsed .ascn file: an EntityData tree saved from the scene editor.
             *        A file whose root is an Entity::Scene is a scene, any other root type is a prefab.
             *        Load it with ResourceManager::getResource<PackedScene>(path).
             */
            struct PackedScene {
                ecs::entities::EntityData data;

                /**
                 * @brief Check if the file's root is an Entity::Scene.
                 *
                 * @return true The file is a scene.
                 * @return false The file is a prefab.
                 */
                bool isScene() const;

                /**
                 * @brief Create the entity tree in @p world and return its root, without a parent.
                 *        The root keeps its flecs default name (unique), so it can be attached anywhere:
                 *        the name saved in the file is data.name, to give once the root is attached.
                 *
                 * @param world World to create the entities in.
                 * @return std::shared_ptr<ecs::entities::Entity> The root, or nullptr if its type is unknown.
                 */
                std::shared_ptr<ecs::entities::Entity> instantiate(flecs::world &world) const;
            };

            class PackedSceneLoader : public Resource<PackedScene>
            {
            public:
                PackedSceneLoader();
                ~PackedSceneLoader() override;

                std::shared_ptr<PackedScene> load(const std::string &path) override;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
