#include <exception>

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
                m_texture.clear();
            }

            void ImageLoader::load(const std::string &path)
            {
                try {
                    m_texture = std::string("test string loaded");
                }
                catch (const std::exception &e) {
                    std::string expCatch = e.what();
                    throw LoadException("catched " + expCatch + "during font loading");
                }
            }

            std::any ImageLoader::get()
            {
                return std::make_any<std::string>(m_texture);
            }

            void ImageLoader::destroy()
            {
                m_texture.clear();
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
