#include "prefab_registry.hpp"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            void PrefabRegistry::AddPrefabLoader(const std::function<Prefab(flecs::world)> &loader)
            {
                Instance().m_prefab_loaders.push_back(loader);
            }

            const std::vector<std::function<Prefab(flecs::world)>> &PrefabRegistry::GetLoaders()
            {
                return Instance().m_prefab_loaders;
            }

            PrefabRegistry &PrefabRegistry::Instance()
            {
                static PrefabRegistry instance;

                return instance;
            }
        } // namespace ecs
    } // namespace core
} // namespace atmo
