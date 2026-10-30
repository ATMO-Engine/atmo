#include "prefab.hpp"
#include <exception>
#include <format>
#include <string>
#include "core/ecs/entity_registry.hpp"
#include "glaze/glaze.hpp"
#include "project/file_system.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs
{
    static std::string UniqueChildName(const entities::Entity &parent, const std::string &name)
    {
        std::string candidate = name;
        for (int i = 2; parent.getHandle().lookup(candidate.c_str()).is_valid(); ++i)
            candidate = std::format("{}_{}", name, i);
        return candidate;
    }

    std::shared_ptr<entities::Entity> Prefab::Instantiate(std::string_view path, entities::Entity parent)
    {
        if (!parent.isAlive()) {
            spdlog::error("Prefab::Instantiate: invalid parent for '{}'", path);
            return nullptr;
        }

        entities::EntityData data;
        try {
            project::File file = project::FileSystem::OpenFile(path);
            const std::string content = file.readAll();
            if (auto err = glz::read_json(data, content); err) {
                spdlog::error("Prefab::Instantiate: failed to parse '{}': {}", path, glz::format_error(err, content));
                return nullptr;
            }
        } catch (const std::exception &e) {
            spdlog::error("Prefab::Instantiate: could not open '{}': {}", path, e.what());
            return nullptr;
        }

        flecs::world world = parent.getHandle().world();
        auto root = EntityRegistry::CreateIn(&world, data.type);
        if (!root)
            return nullptr;

        const std::string prefab_name = std::move(data.name);
        data.name = std::string(root->name());
        root->deserializeInWorld(data, &world);
        root->setParent(parent);
        root->rename(UniqueChildName(parent, prefab_name));

        return root;
    }
} // namespace atmo::core::ecs
