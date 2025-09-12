#include "core/resource/loaders/image_loader.hpp"
#include <any>
#include <memory>
#include <string>
#include <utility>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ImageLoader::ImageLoader() {}

            void ImageLoader::load(const std::string &path) {}

            std::any ImageLoader::get(const std::string &path)
            {
                std::shared_ptr<std::string> newRessource = std::make_shared<std::string>("test");

                if (ressources.find(path) != ressources.end()) {
                    ressources.insert(std::make_pair(path, newRessource));
                }

                return ressources.at(path);
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
