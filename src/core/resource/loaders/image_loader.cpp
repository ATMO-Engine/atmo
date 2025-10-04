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
                _texture.clear();
            }

            void ImageLoader::load(const std::string &path)
            {
                try {
                    _texture = std::string("test string loaded");
                }
                catch (const std::exception &e) {
                    throw e;
                }
            }

            std::any ImageLoader::get()
            {
                return std::make_any<std::string>(_texture);
            }

            void ImageLoader::destroy()
            {
                _texture.clear();
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
