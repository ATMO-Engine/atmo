#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/resource/loaders/Ipool.hpp"
#include "handle.hpp"
#include "loaders/Ipool.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ResourceManager
            {
            public:
                static ResourceManager &getInstance();

                ~ResourceManager() = default;

                /**
                 * @brief If needed generate the resource associated to the path given and give the Handle
                 *
                 * @param path absolute path of the resource you want to load
                 * @return Handle handle associated with the resource
                 */
                const Handle &generate(const std::string &path);

                /**
                 * @brief get the resource associated to the handle if possible,
                 *        throw an exception if the handle is outdated
                 *
                 * @param handle handle associated to the ressource you want to get
                 * @return std::any ressource ready to use
                 */
                std::any getResource(const Handle &handle); //TODO: créer l'exception pour les handle périmé
            private:
                ResourceManager();
                ResourceManager &operator=(const ResourceManager &) = delete;
                std::vector<std::string> split(const std::string &str, char delimiter);

                std::unordered_map<std::string, Handle> _handleMap;
                std::unordered_map<ResourceType, std::unique_ptr<IPool>> _pools;
                const std::unordered_map<std::string, ResourceType> _fileTypes;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
