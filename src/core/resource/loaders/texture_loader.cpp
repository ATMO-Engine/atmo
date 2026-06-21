#include "texture_loader.hpp"
#include <memory>
#include "SDL3_image/SDL_image.h"
#include "core/resource/resource_manager.hpp"
#include "project/file_system.hpp"

namespace atmo::core::resource
{

    TextureLoader::TextureLoader() {}

    TextureLoader::~TextureLoader() {}

    std::shared_ptr<SDL_Texture> TextureLoader::load(const std::string &path)
    {
        SDL_Renderer *renderer = ResourceManager::GetInstance().getRenderer();
        if (!renderer) {
            throw LoadException("No renderer available — call ResourceManager::setRenderer before loading textures");
        }

        auto file = project::FileSystem::OpenFile(path);
        SDL_Surface *surface = IMG_Load_IO(file.toIOStream(), true);

        if (!surface) {
            throw LoadException(std::string("Failed to load image: ") + SDL_GetError());
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (!texture) {
            throw LoadException(std::string("Failed to create texture: ") + SDL_GetError());
        }

        return std::shared_ptr<SDL_Texture>(texture, [](SDL_Texture *t) {
            if (t) {
                SDL_DestroyTexture(t);
            }
        });
    }

    const std::string TextureLoader::resourceTypeName()
    {
        return "Image (GPU & VRAM)";
    }
} // namespace atmo::core::resource
