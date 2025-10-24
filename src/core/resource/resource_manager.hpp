#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/resource/pool.hpp"
#include "core/resource/handle.hpp"
#include "core/resource/resource.hpp"
#include "core/resource/resource_factory.hpp"

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
                const Handle generate(const std::string &path);

                /**
                 * @brief get the resource associated to the handle if possible,
                 *        throw an exception if the handle is outdated
                 *
                 * @param handle handle associated to the ressource you want to get
                 * @return std::any ressource ready to use
                 */
                std::shared_ptr<Resource> getResource(const Handle &handle); //TODO: créer l'exception pour les handle périmé

                /**
                 * @brief Declare an handle that is still in use
                 *
                 * @param handle handle to be stored inside his pool
                 */
                void declareHandle(const Handle &handle);
            private:
                ResourceManager();
                ResourceManager &operator=(const ResourceManager &) = delete;

                ResourceFactory &_factory;
                std::unordered_map<std::string, Pool> _pools;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
