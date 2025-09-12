#include "core/resource/loaders/image_loader.hpp"
#include <any>
#include <memory>
#include <string>
#include <utility>

#include <iostream>

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
                std::shared_ptr<std::string> newRessource = std::make_shared<std::string>("test string loaded");

                if (_ressources.find(path) == _ressources.end()) {
                    _ressources.insert(std::make_pair(path, newRessource));
                }
                return std::make_any<std::shared_ptr<std::string>>(_ressources.at(path));
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
