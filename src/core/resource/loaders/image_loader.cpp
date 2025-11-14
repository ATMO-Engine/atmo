#include <exception>
#include "SDL3_image/SDL_image.h"

#include "core/resource/loaders/image_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            LoaderRegister<ImageLoader> ImageLoader::_register("png");

            ImageLoader::ImageLoader() {}

            ImageLoader::~ImageLoader()
            {
                destroy();
            }

            void ImageLoader::load(const std::string &path)
            {
                try {
                    _surface = IMG_Load(path.c_str());
                    if (!_surface) {
                        throw std::runtime_error(std::string("Failed to load image: ") + SDL_GetError());
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::any ImageLoader::get()
            {
                return std::make_any<SDL_Surface *>(_surface);
            }

            void ImageLoader::destroy()
            {
                if (_surface) {
                    SDL_DestroySurface(_surface);
                    _surface = nullptr;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
