#include "font_loader.hpp"

#include <cstddef>
#include <vector>

#include "SDL3/SDL_iostream.h"
#include "core/resource/auto_register_loader.hpp"
#include "project/file_system.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::core::resource
{
    std::shared_ptr<Font> FontLoader::load(const std::string &path)
    {
        auto &server = server::rendering::RenderingServer::Instance();
        if (!server.isInitialized())
            throw LoadException("the RenderingServer is not initialized");

        auto file = project::FileSystem::OpenFile(path);
        SDL_IOStream *io = file.toIOStream();
        if (!io)
            throw LoadException("Failed to open font " + path);

        std::size_t size = 0;
        void *data = SDL_LoadFile_IO(io, &size, true);
        if (!data)
            throw LoadException("Failed to read font " + path);
        std::vector<std::byte> bytes(static_cast<std::byte *>(data), static_cast<std::byte *>(data) + size);
        SDL_free(data);

        const server::rendering::FontRid rid = server.fontCreate(bytes, path);
        if (!rid.isValid())
            throw LoadException("Failed to load font " + path);
        return std::make_shared<Font>(rid);
    }
} // namespace atmo::core::resource

ATMO_REGISTER_RESOURCE_LOADER(atmo::core::resource::Font, atmo::core::resource::FontLoader, .display_name = "Font", .extensions = { ".ttf", ".otf" });
