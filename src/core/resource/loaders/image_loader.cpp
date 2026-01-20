#include <exception>
#include <memory>
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

            std::shared_ptr<SDL_Surface> ImageLoader::load(const std::string &path)
            {
                try {
                    SDL_Surface *surface = IMG_Load(path.c_str());
                    if (!surface) {
                        throw std::runtime_error(std::string("Failed to load image: ") + SDL_GetError());
                    }
                    return std::shared_ptr<SDL_Surface>(surface, [](SDL_Surface *s) {
                        if (s) {
                            SDL_DestroySurface(s);
                        }
                    });
                } catch (const std::exception &e) {
                    throw e;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
