#include <exception>
#include <sstream>

#include "resource_manager.hpp"
#include "core/resource/loaders/image_loader.hpp"

using namespace atmo::core::resource;

const std::map<std::string, ResourceLoader> _loaders = {
    {"png", ImageLoader()},
    {"jpeg", ImageLoader()}
    //{"ttf", FontLoader}
};

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            std::vector<std::string> ResourceManager::split(const std::string& str, char delimiter) {
                std::vector<std::string> tokens;
                std::istringstream stream(str);
                std::string token;

                while (std::getline(stream, token, delimiter)) {
                    tokens.push_back(token);
                }

                return tokens;
            }

            ResourceLoader &ResourceManager::getResources(std::string &path) {
                std::string &extension = split(path, '.').back();
                try {
                    return _loaders.at(extension).get(path);
                } catch (const std::exception &e) {
                    throw e;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
