#include "texture_loader.hpp"
#include <memory>
#include "SDL3_image/SDL_image.h"
#include "core/resource/auto_register_loader.hpp"
#include "project/file_system.hpp"

namespace atmo::core::resource
{

    TextureLoader::TextureLoader() {}

    TextureLoader::~TextureLoader() {}

    std::shared_ptr<SDL_Texture> TextureLoader::loadWithContext(const std::string &path, SDL_Renderer *renderer)
    {
        if (!renderer) {
            throw LoadException("No renderer provided to load this texture against");
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
} // namespace atmo::core::resource

ATMO_REGISTER_RESOURCE_LOADER(
    SDL_Texture, atmo::core::resource::TextureLoader, .display_name = "Image (GPU & VRAM)", .extensions = { ".png", ".jpg", ".jpeg", ".bmp" });
