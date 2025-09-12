#include "core/resource/loaders/image_loader.hpp"
#include <string>
#include <utility>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ImageLoader::ImageLoader() {}

            void ImageLoader::load(const std::string &path)
            {
                std::shared_ptr<std::string> newRessource = std::make_shared<std::string>("test string loaded");

                _resources.insert(std::make_pair(path, newRessource));
            }

            std::any ImageLoader::get(const std::string &path)
            {
                if (_resources.find(path) == _resources.end()) {
                    load(path);
                }

                return std::make_any<std::shared_ptr<std::string>>(_resources.at(path));
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
