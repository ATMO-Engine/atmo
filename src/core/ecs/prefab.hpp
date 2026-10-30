#pragma once

#include <memory>
#include <string_view>
#include "core/ecs/entities/entity.hpp"

namespace atmo::core::ecs
{
    class Prefab
    {
    public:
        /**
         * @brief Load a prefab file and create its entity tree under @p parent, in the parent's world.
         *        If a sibling already has the prefab root's name, the root is renamed "<name>_2", "<name>_3", ...
         *
         * @param path Path of the .prefab file.
         * @param parent Entity the prefab root is attached to.
         * @return std::shared_ptr<entities::Entity> The prefab root, or nullptr if the file could not be loaded.
         */
        static std::shared_ptr<entities::Entity> Instantiate(std::string_view path, entities::Entity parent);
    };
} // namespace atmo::core::ecs
