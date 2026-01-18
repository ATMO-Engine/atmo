#include <exception>
#include <stdexcept>
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

            SDL_Surface *ImageLoader::load(const std::string &path)
            {
                try {
                    SDL_Surface *surface = IMG_Load(path.c_str());
                    if (!surface) {
                        throw std::runtime_error(std::string("Failed to load image: ") + SDL_GetError());
                    }
                    return surface;
                }
                catch (const std::exception &e) {
                    throw e;
                }
            }

            void ImageLoader::destroy(SDL_Surface *res)
            {
                if (res) {
                    SDL_DestroySurface(res);
                    res = nullptr;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
