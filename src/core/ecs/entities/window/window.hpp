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

        void setName(const std::string &name);
        void setSize(const core::types::Vector2i &size);
        void focus();

        core::types::Vector2i getSize() const noexcept;
        std::string getTitle() const noexcept;

        SDL_Texture *getTextureFromHandle(const std::string &path);

        /**
         * @brief Set a callback function to be called when the window is closed. This can be used to perform cleanup tasks or prompt the user for confirmation
         * before closing.
         *
         * @param callback Function to be called when the window is closed.
         */
        void onClose(std::function<void()> callback);

    private:
        void pollEvents(float deltaTime);
        void beginDraw(components::Window &window);
        void draw(components::Window &window);

        Clay_ElementId getIdForEntity(flecs::entity e);
        Clay_ElementDeclaration buildDecl(flecs::entity e);
        void declareEntityUi(flecs::entity e);
    };
} // namespace atmo::core::ecs::entities
