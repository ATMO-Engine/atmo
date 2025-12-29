#include <utility>
#include <spdlog/spdlog.h>

#include "core/resource/resource_factory.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            using Loader = std::function<std::shared_ptr<Resource>()>;

            ResourceFactory::ResourceFactory() {}

            ResourceFactory &ResourceFactory::GetInstance()
            {
                static ResourceFactory instance;
                return instance;
            }

            void ResourceFactory::registerLoader(const std::string &fileExtension, Loader loader)
            {
                if (m_loaders.find(fileExtension) == m_loaders.end()) {
                    m_loaders.insert(std::make_pair(fileExtension, loader));
                } else {
                    spdlog::warn("Loader for this extenstion {} is already registered", fileExtension);
                }
            }

            std::shared_ptr<Resource> ResourceFactory::create(const std::string &fileExtension)
            {
                auto it = m_loaders.find(fileExtension);
                if (it != m_loaders.end()) {
                    return (it->second());
                } else {
                    return nullptr;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
