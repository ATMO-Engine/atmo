#include "impl/window.hpp"
#include "core/ecs/ecs.hpp"
#include "core/ecs/prefab_registry.hpp"
#include "flecs.h"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

atmo::core::ecs::Prefab createWindowPrefab(flecs::world world)
{
    using namespace atmo::core;
    auto windowPrefab = ecs::Prefab(world, "window").managed<atmo::impl::WindowManager>(components::Window{ "Atmo Managed Window", { 800, 600 } });

    world.system<ComponentManager::Managed, components::Window>("PollEvents")
        .kind(flecs::PreUpdate)
        .each([](flecs::iter &it, size_t i, ComponentManager::Managed &manager, components::Window &window) {
            auto wm = static_cast<atmo::impl::WindowManager *>(manager.ptr);
            wm->pollEvents(it.delta_time());
            wm->beginDraw();
        });

    world.system<ComponentManager::Managed, components::Window>("Draw")
        .kind(flecs::PostUpdate)
        .each([](ComponentManager::Managed &manager, components::Window &window) {
            auto wm = static_cast<atmo::impl::WindowManager *>(manager.ptr);
            wm->draw();
        });

    return windowPrefab;
}

namespace
{
    static int _ = [] {
        atmo::core::ecs::PrefabRegistry::AddPrefabLoader(&createWindowPrefab);
        return 0;
    }();
} // namespace
