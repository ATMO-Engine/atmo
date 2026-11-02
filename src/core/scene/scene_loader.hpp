#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/resource/loaders/packed_scene_loader.hpp"
#include "flecs.h"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            /**
             * @brief Single entry point to create entities from .ascn files, scenes and prefabs alike.
             *        Every function logs its errors and returns nullptr / false on failure.
             */
            class SceneLoader
            {
            public:
                /**
                 * @brief Create a prefab's entity tree under @p parent, in the parent's world.
                 *        The root gets the name saved in the file, made unique among @p parent's children.
                 *        Scene files are refused: a Scene owns its own physics world and can't be nested.
                 *
                 * @param path Path of the .ascn prefab file.
                 * @param parent Entity the prefab root is attached to.
                 * @return std::shared_ptr<ecs::entities::Entity> The prefab root, or nullptr on failure.
                 */
                static std::shared_ptr<ecs::entities::Entity> InstantiatePrefab(std::string_view path, ecs::entities::Entity parent);

                /**
                 * @brief Create a new Scene from a scene file in @p world, without a parent.
                 *        The scene keeps its flecs default name: the caller gives it @p scene_name once it is attached.
                 *
                 * @param path Path of the .ascn scene file.
                 * @param world World to create the scene in.
                 * @param scene_name Set to the name saved in the file, left untouched on failure.
                 * @return std::shared_ptr<ecs::entities::Scene> The created scene, or nullptr on failure.
                 */
                static std::shared_ptr<ecs::entities::Scene> InstantiateScene(std::string_view path, flecs::world &world, std::string &scene_name);

                /**
                 * @brief Replace @p scene's children with a scene file's content.
                 *
                 * @param path Path of the .ascn scene file.
                 * @param scene Existing scene to load into.
                 * @return true on success, false on failure.
                 */
                static bool LoadSceneInto(std::string_view path, ecs::entities::Scene &scene);

            private:
                /**
                 * @brief Load the parsed file through the ResourceManager and check that it is the expected kind of file.
                 *
                 * @param path Path of the .ascn file.
                 * @param expect_scene true if the file must be a scene, false if it must be a prefab.
                 * @return std::shared_ptr<resource::PackedScene> The parsed file, or nullptr on failure.
                 */
                static std::shared_ptr<resource::PackedScene> Load(std::string_view path, bool expect_scene);
            };
        } // namespace scene
    } // namespace core
} // namespace atmo
