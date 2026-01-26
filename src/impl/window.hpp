#pragma once

#include <SDL3/SDL.h>
#include <clay.h>
#include <flecs.h>
#include <memory>
#include <string>

#include "clay_types.hpp"
#include "core/ecs/components.hpp"
#include "core/resource/handle.hpp"
#include "core/resource/resource_manager.hpp"
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

            void pollEvents(float deltaTime);
            void beginDraw();
            void draw();

            void rename(const std::string &name) noexcept;
            void resize(const core::types::vector2i &size) noexcept;
            void focus() noexcept;
            void makeMain() noexcept;

            core::types::vector2i getSize() const noexcept;
            std::string getTitle() const noexcept;

            inline SDL_Renderer *getRenderer() const noexcept
            {
                return m_rendererData.renderer;
            }

            inline SDL_Texture *getTextureFromHandle(const core::resource::Handle<SDL_Surface> &handle)
            {
                if (m_textureCache.find(handle) != m_textureCache.end()) {
                    return m_textureCache[handle];
                }

                atmo::core::resource::ResourceRef<SDL_Surface> res =
                    atmo::core::resource::ResourceManager::GetInstance().getResource<SDL_Surface>(handle.assetId);
                std::shared_ptr<SDL_Surface> surface = res.get();

                SDL_Texture *texture = SDL_CreateTextureFromSurface(m_rendererData.renderer, surface.get());
                m_textureCache[handle] = texture;
                return texture;
            }

        private:
            Clay_ElementDeclaration buildDecl(flecs::entity e);
            Clay_ElementId getIdForEntity(flecs::entity e);
            void declareEntityUi(flecs::entity e);

            SDL_Window *m_window = nullptr;
            Clay_SDL3RendererData m_rendererData;
            Clay_Arena m_clayArena;
            std::map<core::resource::Handle<SDL_Surface>, SDL_Texture *> m_textureCache;
        };
    } // namespace impl
} // namespace atmo
