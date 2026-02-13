#include <exception>
#include <memory>
#include "SDL3_image/SDL_image.h"
#include "image_loader.hpp"
#include "project/file_system.hpp"

#include "core/resource/loaders/image_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ImageLoader::ImageLoader() {}

            ImageLoader::~ImageLoader() {}

            std::shared_ptr<SDL_Surface> ImageLoader::load(const std::string &path)
            {
                auto file = project::FileSystem::OpenFile(path);
                SDL_Surface *surface = IMG_Load_IO(file.toIOStream(), true);

                if (!surface) {
                    throw LoadException(std::string("Failed to load image: ") + SDL_GetError());
                }
                return std::shared_ptr<SDL_Surface>(surface, [](SDL_Surface *s) {
                    if (s) {
                        SDL_DestroySurface(s);
                    }
                });
            }

            const std::string ImageLoader::resourceTypeName() {
                return "Image (SDL Surface)";
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
