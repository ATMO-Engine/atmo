#pragma once

#include <SDL3/SDL.h>
#include <clay.h>
#include <flecs.h>
#include <string>

#include "clay_types.hpp"
#include "core/ecs/components.hpp"
#include "core/types.hpp"

// macro resulting to SDL_WINDOW_VULKAN on windows and linux, SDL_WINDOW_METAL on macOS
#define GET_RENDERER_FLAGS()

namespace atmo
{
    namespace impl
    {
        class WindowManager : public core::ComponentManager
        {
        public:
            WindowManager(flecs::entity entity);
            ~WindowManager();

            static void RegisterSystems(flecs::world ecs)
            {
                ecs.system<core::ComponentManager::Managed, core::components::Window>("PollEvents")
                    .kind(flecs::PreUpdate)
                    .each([](flecs::iter &it, size_t i, core::ComponentManager::Managed &manager, core::components::Window &window) {
                        auto *wm = static_cast<impl::WindowManager *>(manager.ptr);
                        wm->pollEvents(it.delta_time());
                    });

                ecs.system<core::ComponentManager::Managed, core::components::Window>("Draw")
                    .kind(flecs::PostUpdate)
                    .each([](core::ComponentManager::Managed &manager, core::components::Window &window) {
                        auto *wm = static_cast<impl::WindowManager *>(manager.ptr);
                        wm->draw();
                    });
            }

            void pollEvents(float deltaTime);
            void draw();

            void rename(const std::string &name) noexcept;
            void resize(const core::types::vector2i &size) noexcept;
            void focus() noexcept;
            void make_main() noexcept;

            core::types::vector2i getSize() const noexcept;
            std::string getTitle() const noexcept;

        private:
            Clay_ElementDeclaration BuildDecl(flecs::entity e);
            Clay_ElementId getIdForEntity(flecs::entity e);
            void DeclareEntityUI(flecs::entity e);

            static inline flecs::entity main_window;

            SDL_Window *window = nullptr;
            Clay_SDL3RendererData rendererData;
            Clay_Arena clay_arena;
        };
    } // namespace impl
} // namespace atmo
