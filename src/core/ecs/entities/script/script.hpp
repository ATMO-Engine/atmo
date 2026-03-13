#pragma once

#include <string_view>
#include "core/ecs/components.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    class Script : public EntityRegistry::Registrable<Script, Entity>
    {
    public:
        using EntityRegistry::Registrable<Script, Entity>::Registrable;

        static void RegisterComponents(flecs::world *world);
        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Script";
        }

        void setScriptInstance(luau::ScriptInstance *instance);
        void setScriptPath(const std::string &path);
        std::string_view getScriptPath() const noexcept;
    };
} // namespace atmo::core::ecs::entities
