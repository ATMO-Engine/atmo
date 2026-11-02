#pragma once

#include <functional>
#include <optional>
#include <string>

#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "impl/clay_types.hpp"
#include "meta/meta.hpp"
#include "server/rendering/rid.hpp"

namespace atmo::core::components
{
    struct Window {
        std::string title;
        types::Vector2i size;
        types::Vector2 dpi_scale;
        server::rendering::WindowRid rid;
        server::rendering::CanvasRid ui_canvas;
        server::rendering::CanvasItemRid ui_item;
        Clay_Context *clay_context = nullptr;
        Clay_Arena clay_arena{};
        std::optional<std::function<void()>> close_callback;
        bool headless = false;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Window> {
    static constexpr const char *name = "Window";
    static constexpr auto fields =
        std::make_tuple(atmo::meta::field<&atmo::core::components::Window::title>("title"), atmo::meta::field<&atmo::core::components::Window::size>("size"));
};

namespace atmo::core::ecs::entities
{
    class Window : public EntityRegistry::Registrable<Window, Entity>
    {
    public:
        using EntityRegistry::Registrable<Window, Entity>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Window";
        }

        bool setTitle(const std::string &name);
        void setSize(const core::types::Vector2i &size);
        void focus();

        core::types::Vector2i getSize() const noexcept;
        std::string getTitle() const noexcept;

        types::Vector2 getDPIScale() const noexcept;

        void onClose(std::function<void()> callback);

        void primeScrollContainers();

    private:
        void draw(components::Window &window);

        Clay_ElementId getIdForEntity(const Entity &e);
        Clay_ElementDeclaration buildDecl(const Entity &e);
    };
} // namespace atmo::core::ecs::entities
