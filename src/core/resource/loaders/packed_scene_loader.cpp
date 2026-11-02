#include "packed_scene_loader.hpp"
#include <format>
#include <memory>
#include <string>
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/auto_register_loader.hpp"
#include "glaze/glaze.hpp"
#include "project/file_system.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            bool PackedScene::isScene() const
            {
                return data.type == ecs::entities::Scene::FullName();
            }

            std::shared_ptr<ecs::entities::Entity> PackedScene::instantiate(flecs::world &world) const
            {
                auto root = ecs::EntityRegistry::CreateIn(&world, data.type);
                if (!root)
                    return nullptr;

                ecs::entities::EntityData root_data = data;
                root_data.name = std::string(root->name());
                root->deserializeInWorld(root_data, &world);

                return root;
            }

            PackedSceneLoader::PackedSceneLoader() {}

            PackedSceneLoader::~PackedSceneLoader() {}

            std::shared_ptr<PackedScene> PackedSceneLoader::load(const std::string &path)
            {
                auto file = project::FileSystem::OpenFile(path);
                const std::string content = file.readAll();

                auto packed_scene = std::make_shared<PackedScene>();
                if (auto err = glz::read_json(packed_scene->data, content); err)
                    throw LoadException(std::format("Failed to parse '{}': {}", path, glz::format_error(err, content)));

                return packed_scene;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo

ATMO_REGISTER_RESOURCE_LOADER(atmo::core::resource::PackedScene, atmo::core::resource::PackedSceneLoader, .display_name = "Scene", .extensions = { ".ascn" });
