#pragma once

#include <memory>
#include <string_view>
#include "core/ecs/components.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/loaders/script_loader.hpp"
#include "core/resource/resource_ref.hpp"
#include "luau/script_instance.hpp"

namespace atmo::core::components
{
    struct ScriptTest {
        std::string script_path;
        std::unique_ptr<resource::ResourceRef<resource::Bytecode>> m_res;
        atmo::luau::ScriptInstance *instance = nullptr;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::ScriptTest> {
    static constexpr const char *name = "ScriptTest";
    static constexpr const char *category = "Luau";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::ScriptTest::script_path>("script_path").withWidget("file_path"));
};

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
