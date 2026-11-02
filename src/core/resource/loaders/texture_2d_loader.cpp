#include "texture_2d_loader.hpp"

#include <cstring>
#include <vector>

#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_surface.h"
#include "SDL3_image/SDL_image.h"
#include "core/resource/auto_register_loader.hpp"
#include "project/file_system.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::core::resource
{
    std::shared_ptr<Texture2d> Texture2dLoader::load(const std::string &path)
    {
        auto &server = server::rendering::RenderingServer::Instance();
        if (!server.isInitialized())
            throw LoadException("the RenderingServer is not initialized");

        auto file = project::FileSystem::OpenFile(path);
        SDL_Surface *decoded = IMG_Load_IO(file.toIOStream(), true);
        if (!decoded)
            throw LoadException(std::string("Failed to load image: ") + SDL_GetError());

        SDL_Surface *surface = SDL_ConvertSurface(decoded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(decoded);
        if (!surface)
            throw LoadException(std::string("Failed to convert image: ") + SDL_GetError());

        const types::Vector2i size{ surface->w, surface->h };
        const std::size_t row = static_cast<std::size_t>(surface->w) * 4;
        std::vector<std::byte> pixels(row * static_cast<std::size_t>(surface->h));
        for (int y = 0; y < surface->h; ++y)
            std::memcpy(
                pixels.data() + (static_cast<std::size_t>(y) * row),
                static_cast<const std::byte *>(surface->pixels) + (static_cast<std::size_t>(y) * surface->pitch),
                row);
        SDL_DestroySurface(surface);

        const server::rendering::TextureRid rid = server.texture2dCreate({ .size = size, .debug_name = path }, pixels);
        if (!rid.isValid())
            throw LoadException("Failed to create the texture for " + path);

        return std::make_shared<Texture2d>(rid, size);
    }
} // namespace atmo::core::resource

ATMO_REGISTER_RESOURCE_LOADER(
    atmo::core::resource::Texture2d, atmo::core::resource::Texture2dLoader, .display_name = "Image (RenderingServer)",
    .extensions = { ".png", ".jpg", ".jpeg", ".bmp" });
