#include <exception>
#include <memory>
#include "SDL3_image/SDL_image.h"

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
                SDL_Surface *surface = nullptr;
                try {
                    surface = IMG_Load(path.c_str());
                } catch (const std::exception &e) {
                    throw e;
                }

                if (!surface) {
                    throw LoadException(std::string("Failed to load image: ") + SDL_GetError());
                }
                return std::shared_ptr<SDL_Surface>(surface, [](SDL_Surface *s) {
                    if (s) {
                        SDL_DestroySurface(s);
                    }
                });
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
