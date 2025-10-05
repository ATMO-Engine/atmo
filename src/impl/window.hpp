#pragma once

#include <SDL3/SDL.h>
#include <clay.h>
#include <flecs.h>
#include <string>

#include "clay_types.hpp"
#include "core/components.hpp"
#include "core/types.hpp"

namespace atmo
{
    namespace impl
    {
        class WindowManager : public core::ComponentManager
        {
        public:
            WindowManager(const atmo::core::components::Window &window);
            ~WindowManager();

            template <typename Component>
            static void registerSystems(
                flecs::world &ecs, std::unordered_map<flecs::entity_t, ComponentManager *> component_managers
            )
            {
                ecs.system<Component>("PollEvents")
                    .kind(flecs::PreUpdate)
                    .each([&component_managers](flecs::iter &it, size_t, core::components::Window &window) {
                        impl::WindowManager *wm =
                            static_cast<impl::WindowManager *>(component_managers[it.entity(0).id()]);
                        wm->pollEvents(it.delta_time());
                    });

                ecs.system<Component>("Draw")
                    .kind(flecs::PostUpdate)
                    .each([&component_managers](flecs::iter &it, size_t, core::components::Window &window) {
                        impl::WindowManager *wm =
                            static_cast<impl::WindowManager *>(component_managers[it.entity(0).id()]);
                        wm->draw();
                    });
            }

            void pollEvents(float deltaTime);
            void draw();

            void rename(const std::string &name) noexcept;
            void resize(const core::types::vector2i &size) noexcept;

            core::types::vector2i getSize() const noexcept;
            std::string getTitle() const noexcept;

        private:
            SDL_Window *window = nullptr;
            Clay_SDL3RendererData rendererData;
            Clay_Arena clay_arena;
            bool is_main = false;
        };
    } // namespace impl
} // namespace atmo
#
