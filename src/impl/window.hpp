#pragma once

#include <SDL3/SDL.h>
#include <clay.h>
#include <flecs.h>
#include <string>

#include "clay_types.hpp"
#include "core/ecs/components.hpp"
#include "core/types.hpp"

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
            void makeMain() noexcept;

            core::types::vector2i getSize() const noexcept;
            std::string getTitle() const noexcept;

        private:
            Clay_ElementDeclaration buildDecl(flecs::entity e);
            Clay_ElementId getIdForEntity(flecs::entity e);
            void declareEntityUi(flecs::entity e);

            static inline flecs::entity main_window;

            SDL_Window *m_window = nullptr;
            Clay_SDL3RendererData m_rendererData;
            Clay_Arena m_clayArena;
        };
    } // namespace impl
} // namespace atmo
