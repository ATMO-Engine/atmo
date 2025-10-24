#pragma once

#include <exception>
#include <stdexcept>
#include <utility>

#include "core/resource/resource_factory.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            using Loader = std::function<std::shared_ptr<Resource>()>;

            ResourceFactory::ResourceFactory() {}

            ResourceFactory &ResourceFactory::getInstance()
            {
                static ResourceFactory instance;
                return instance;
            }

            void ResourceFactory::registerLoader(const std::string &fileExtension, Loader loader)
            {
                if (_loaders.find(fileExtension) == _loaders.end()) {
                    _loaders.insert(std::make_pair(fileExtension, loader));
                } else {
                    throw std::runtime_error("Loader for this extension already registered");
                }
            }

            std::shared_ptr<Resource> ResourceFactory::create(const std::string &fileExtension)
            {
                auto it = _loaders.find(fileExtension);
                if (it != _loaders.end()) {
                    return (it->second());
                } else {
                    return nullptr;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
