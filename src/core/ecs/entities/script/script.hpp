#pragma once

#include <string_view>
#include "core/ecs/components.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/handle.hpp"
#include "core/resource/loaders/script_loader.hpp"
#include "luau/script_instance.hpp"

namespace atmo::core::components
{
    struct ScriptTest {
        std::string script_path;
        atmo::core::resource::Handle<resource::Bytecode> m_handle;
        atmo::luau::ScriptInstance *instance = nullptr;
    };
} // namespace atmo::core::components

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
