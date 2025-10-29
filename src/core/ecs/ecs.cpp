#include "ecs.hpp"
#include "impl/window.hpp"

atmo::core::ecs::ECS::ECS() : m_world()
{
    ECS_IMPORT(m_world, FlecsMeta);

    m_world.init_builtin_components();
    components::register_core_components(m_world);
    loadPrefabs();
}

void atmo::core::ecs::ECS::stop()
{
    m_world.quit();
}

void atmo::core::ecs::ECS::loadPrefabs()
{
    m_prefabs.emplace("window", createManagedPrefab<impl::WindowManager>("window").set<components::Window>({ "Atmo Managed Window", { 800, 600 } }));

    // { // UI
    //     // rect should have:
    //     // - Background color
    //     // - Image
    //     // - Border
    //     // - Child alignment
    //     // - Padding
    //     prefabs.emplace("ui.rect", ecs.prefab("ui.rect"));
    //     prefabs.emplace("ui.label", ecs.prefab("ui.label").set<components::UI::Text>({ "Label", {} }).set<components::UI::UI>({}));
    // }
}

atmo::core::ecs::Entity atmo::core::ecs::ECS::instantiatePrefab(const std::string &name, const std::string &instance_name)
{
    if (m_prefabs.find(name) == m_prefabs.end()) {
        throw std::runtime_error("Prefab not found: " + name);
    }

    flecs::entity instance;
    if (instance_name.empty()) {
        instance = m_world.entity().is_a(m_prefabs.at(name));
    } else {
        instance = m_world.entity(instance_name.c_str()).is_a(m_prefabs.at(name));
    }

    return instance;
}
