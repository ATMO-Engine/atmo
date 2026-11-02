#include "scene_loader.hpp"
#include <exception>
#include "core/resource/resource_manager.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace scene
        {
            std::shared_ptr<resource::PackedScene> SceneLoader::Load(std::string_view path, bool expect_scene)
            {
                std::shared_ptr<resource::PackedScene> packed_scene;
                try {
                    packed_scene = resource::ResourceManager::GetInstance().getResource<resource::PackedScene>(std::string(path));
                } catch (const std::exception &e) {
                    spdlog::error("SceneLoader: could not load '{}': {}", path, e.what());
                    return nullptr;
                }

                if (expect_scene && !packed_scene->isScene()) {
                    spdlog::error("SceneLoader: '{}' is a prefab, not a scene", path);
                    return nullptr;
                }
                if (!expect_scene && packed_scene->isScene()) {
                    spdlog::error("SceneLoader: '{}' is a scene and can't be instantiated under a parent", path);
                    return nullptr;
                }

                return packed_scene;
            }

            std::shared_ptr<ecs::entities::Entity> SceneLoader::InstantiatePrefab(std::string_view path, ecs::entities::Entity parent)
            {
                if (!parent.isAlive()) {
                    spdlog::error("SceneLoader: invalid parent for '{}'", path);
                    return nullptr;
                }

                auto packed_scene = Load(path, false);
                if (!packed_scene)
                    return nullptr;

                flecs::world world = parent.getHandle().world();
                auto root = packed_scene->instantiate(world);
                if (!root)
                    return nullptr;

                root->setParent(parent);
                root->rename(parent.uniqueChildName(packed_scene->data.name));

                return root;
            }

            std::shared_ptr<ecs::entities::Scene> SceneLoader::InstantiateScene(std::string_view path, flecs::world &world, std::string &scene_name)
            {
                auto packed_scene = Load(path, true);
                if (!packed_scene)
                    return nullptr;

                auto root = packed_scene->instantiate(world);
                if (!root)
                    return nullptr;

                scene_name = packed_scene->data.name;
                return std::static_pointer_cast<ecs::entities::Scene>(root);
            }

            bool SceneLoader::LoadSceneInto(std::string_view path, ecs::entities::Scene &scene)
            {
                auto packed_scene = Load(path, true);
                if (!packed_scene)
                    return false;

                scene.loadFromData(packed_scene->data);
                return true;
            }
        } // namespace scene
    } // namespace core
} // namespace atmo
