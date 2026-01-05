#include <exception>

#include "core/resource/loaders/image_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ImageLoader::ImageLoader() {}

            ImageLoader::~ImageLoader() {}

            std::string *ImageLoader::load(const std::string &path)
            {
                try {
                    std::string *texture = new std::string("test string loaded");
                    return texture;
                }
                catch (const std::exception &e) {
                    throw e;
                }
            }

            void ImageLoader::destroy(std::string *res)
            {
                res->clear();
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
