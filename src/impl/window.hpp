#pragma once

#include <SDL3/SDL.h>
#include <clay.h>
#include <flecs.h>
#include <string>

#include "clay_types.hpp"
#include "core/ecs/components.hpp"
#include "core/resource/handle.hpp"
#include "core/resource/resource_manager.hpp"
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

            void pollEvents(float deltaTime);
            void beginDraw();
            void draw();

            void rename(const std::string &name) noexcept;
            void resize(const core::types::vector2i &size) noexcept;
            void focus() noexcept;
            void make_main() noexcept;

            core::types::vector2i getSize() const noexcept;
            std::string getTitle() const noexcept;

            inline SDL_Renderer *getRenderer() const noexcept
            {
                return rendererData.renderer;
            }

            inline SDL_Texture *getTextureFromHandle(const core::resource::Handle &handle)
            {
                if (texture_cache.find(handle) != texture_cache.end()) {
                    return texture_cache[handle];
                }

                auto res = core::resource::ResourceManager::getInstance().getResource(handle);
                auto surface = std::any_cast<SDL_Surface *>(res->get());

                SDL_Texture *texture = SDL_CreateTextureFromSurface(rendererData.renderer, surface);
                texture_cache[handle] = texture;
                return texture;
            }

        private:
            Clay_ElementDeclaration BuildDecl(flecs::entity e);
            Clay_ElementId getIdForEntity(flecs::entity e);
            void DeclareEntityUI(flecs::entity e);

            static inline flecs::entity main_window;

            SDL_Window *window = nullptr;
            Clay_SDL3RendererData rendererData;
            Clay_Arena clay_arena;

            std::map<core::resource::Handle, SDL_Texture *> texture_cache;
        };
    } // namespace impl
} // namespace atmo
