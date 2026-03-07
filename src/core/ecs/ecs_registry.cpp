#include "ecs_registry.hpp"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            void Registry::AddPrefabLoader(const PrefabLoaderFn &loader)
            {
                Instance().m_prefab_loaders.push_back(loader);
            }

            const std::vector<Registry::PrefabLoaderFn> &Registry::GetPrefabLoaders()
            {
                return Instance().m_prefab_loaders;
            }

            void Registry::AddBehaviorLoader(const BehaviorLoaderFn &loader)
            {
                Instance().m_behavior_loaders.push_back(loader);
            }

            const std::vector<Registry::BehaviorLoaderFn> &Registry::GetBehaviorLoaders()
            {
                return Instance().m_behavior_loaders;
            }

            Registry &Registry::Instance()
            {
                static Registry instance;

                return instance;
            }
        } // namespace ecs
    } // namespace core
} // namespace atmo
