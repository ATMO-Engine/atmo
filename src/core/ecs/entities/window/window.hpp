#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>

#include "SDL3/SDL_render.h"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/handle.hpp"
#include "core/types.hpp"
#include "impl/clay_types.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct Window {
        std::string title;
        types::Vector2i size;
        SDL_Window *window = nullptr;
        ClaySdL3RendererData renderer_data;
        Clay_Arena clay_arena;
        std::map<core::resource::Handle<SDL_Surface>, SDL_Texture *> texture_cache;
        std::optional<std::function<void()>> close_callback;
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

        void setTitle(const std::string &name);
        void setSize(const core::types::Vector2i &size);
        void focus();

        core::types::Vector2i getSize() const noexcept;
        std::string getTitle() const noexcept;

        SDL_Texture *getTextureFromHandle(const core::resource::Handle<SDL_Surface> &handle);

        void onClose(std::function<void()> callback);

    private:
        void pollEvents(float deltaTime);
        void beginDraw(components::Window &window);
        void draw(components::Window &window);

        Clay_ElementId getIdForEntity(const Entity &e);
        Clay_ElementDeclaration buildDecl(const Entity &e);
    };
} // namespace atmo::core::ecs::entities
