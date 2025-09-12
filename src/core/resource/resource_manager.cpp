#include <any>
#include <exception>
#include <sstream>

#include "core/resource/loaders/image_loader.hpp"
#include "core/resource/loaders/script_loader.hpp"
#include "resource_manager.hpp"

using namespace atmo::core::resource;


namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ResourceManager::ResourceManager()
            {
                _loaders["png"] = std::make_unique<ImageLoader>();
                _loaders["jpeg"] = std::make_unique<ImageLoader>();
                _loaders["lua"] = std::make_unique<ScriptLoader>();
            };

            std::vector<std::string> ResourceManager::split(const std::string &str, char delimiter)
            {
                std::vector<std::string> tokens;
                std::istringstream stream(str);
                std::string token;

                while (std::getline(stream, token, delimiter)) {
                    tokens.push_back(token);
                }

                return tokens;
            }

            std::any ResourceManager::getResources(std::string &path)
            {
                std::string &extension = split(path, '.').back();
                try {
                    return _loaders.at(extension)->get(path);
                }
                catch (const std::exception &e) {
                    throw e;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
