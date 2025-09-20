#include <any>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <utility>
#include "core/resource/loaders/Ipool.hpp"
#include "handle.hpp"

#include "core/resource/loaders/image_pool.hpp"
#include "core/resource/loaders/script_pool.hpp"
#include "resource_manager.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ResourceManager::ResourceManager() :
                _fileTypes({
                    {"png", ResourceType::PNG},
                    {"luau", ResourceType::Script}
                })
            {
                _pools.emplace(ResourceType::PNG, std::make_unique<ImagePool>(ImagePool()));
                _pools.emplace(ResourceType::Script, std::make_unique<ScriptPool>(ScriptPool()));
            }

            ResourceManager &ResourceManager::getInstance()
            {
                static ResourceManager instance;
                return instance;
            }

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

            const Handle &ResourceManager::generate(const std::string &path)
            {
                if (_handleMap.find(path) != _handleMap.end()) {
                    std::cout << "Already loaded" << std::endl;
                    return _handleMap.at(path);
                }

                std::string extension = split(path, '.').back();
                try {
                    if (_fileTypes.find(extension) != _fileTypes.end()) {
                        ResourceType type = _fileTypes.at(extension);
                        if (_pools.find(type) != _pools.end()) {
                            Handle newHandle = _pools.at(type)->create(path);
                            _handleMap.insert(std::make_pair(path, newHandle));
                            return _handleMap.at(path);
                        } else {
                            throw std::exception(std::format("No pools defined for this file extension ({}).", extension).c_str());
                        }
                    } else {
                        throw std::exception("Invalid file type");
                    }
                } catch (const std::exception &e) {
                    std::cout << e.what() << std::endl;
                    throw e;
                }
            }

            std::any ResourceManager::getResource(const Handle &handle)
            {
                if (_pools.find(handle.type) != _pools.end()) {
                    return _pools.at(handle.type)->getFromHandle(handle);
                    return std::make_any<bool>(true);
                } else {
                    throw std::exception(std::format("No matching pool for the handle given (handle type value:{}).", (int)handle.type).c_str());
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
