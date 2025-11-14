#include <flecs.h>
#include <string>

#include "components.hpp"

/*
static void RegisterSystems(flecs::world ecs)
{
    ecs.system<core::ComponentManager::Managed, core::components::Window>("PollEvents")
        .kind(flecs::PreUpdate)
        .each([](flecs::iter &it, size_t i, core::ComponentManager::Managed &manager, core::components::Window &window) {
            auto *wm = static_cast<impl::WindowManager *>(manager.ptr);
            wm->pollEvents(it.delta_time());
            wm->beginDraw();
        });

    ecs.system<core::ComponentManager::Managed, core::components::Window>("Draw")
        .kind(flecs::PostUpdate)
        .each([](core::ComponentManager::Managed &manager, core::components::Window &window) {
            auto *wm = static_cast<impl::WindowManager *>(manager.ptr);
            wm->draw();
        });
}
*/

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            class Prefab
            {
            public:
                std::string name;
                flecs::entity entity;

                Prefab(const std::string &name) : name(name), entity(flecs::world().prefab(name.c_str())) {}
                ~Prefab() = default;

                template <typename M> inline Prefab &set(const M &component)
                {
                    entity.set<M>(component);
                    return *this;
                }

                template <typename M> inline Prefab &add()
                {
                    entity.add<M>();
                    return *this;
                }

                template <typename Manager, typename Managed> inline Prefab managed(const Managed &component)
                {
                    entity.world()
                        .observer<ComponentManager::Managed>()
                        .event(flecs::OnAdd)
                        .with(flecs::IsA, entity)
                        .each([](flecs::entity e, ComponentManager::Managed &m) { m.ptr = new Manager(e); });

                    entity.world()
                        .observer<ComponentManager::Managed>()
                        .event(flecs::OnRemove)
                        .with(flecs::IsA, entity)
                        .each([](flecs::entity e, ComponentManager::Managed &m) { delete m.ptr; });

                    entity.set(component);

                    return *this;
                }
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
