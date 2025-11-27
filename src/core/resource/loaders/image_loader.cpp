#include <exception>
#include "SDL3_image/SDL_image.h"

#include "core/resource/loaders/image_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            LoaderRegister<ImageLoader> ImageLoader::m_register("png");

            ImageLoader::ImageLoader() : m_surface(nullptr) {}

            ImageLoader::~ImageLoader()
            {
                destroy();
            }

            void ImageLoader::load(const std::string &path)
            {
                try {
                    m_surface = IMG_Load(path.c_str());
                    if (!m_surface) {
                        throw std::runtime_error(std::string("Failed to load image: ") + SDL_GetError());
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::any ImageLoader::get()
            {
                return std::make_any<SDL_Surface *>(m_surface);
            }

            void ImageLoader::destroy()
            {
                if (m_surface) {
                    SDL_DestroySurface(m_surface);
                    m_surface = nullptr;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
