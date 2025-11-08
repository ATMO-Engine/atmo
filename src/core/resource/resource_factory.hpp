#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "core/resource/resource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ResourceFactory
            {
            public:
                static ResourceFactory &getInstance();
                ~ResourceFactory() = default;

                using Loader = std::function<std::shared_ptr<Resource>()>;
                void registerLoader(const std::string &fileExtension, Loader loader);
                std::shared_ptr<Resource> create(const std::string &fileExtension);
            private:
                ResourceFactory();
                std::unordered_map<std::string, Loader> _loaders;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
