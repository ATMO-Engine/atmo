#pragma once

#include "core/ecs/components.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    class Window : public EntityRegistry::Registrable<Window, Entity>
    {
    public:
        using EntityRegistry::Registrable<Window, Entity>::Registrable;

        static void RegisterComponents(flecs::world *world);
        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Window";
        }

        void setName(const std::string &name) noexcept;
        void setSize(const core::types::Vector2i &size) noexcept;
        void focus() noexcept;

        core::types::Vector2i getSize() const noexcept;
        std::string getTitle() const noexcept;

    private:
        void pollEvents(float deltaTime);
        void beginDraw(components::Window &window);
        void draw(components::Window &window);

        Clay_ElementId getIdForEntity(flecs::entity e);
        Clay_ElementDeclaration buildDecl(flecs::entity e);
        void declareEntityUi(flecs::entity e);
    };
} // namespace atmo::core::ecs::entities
