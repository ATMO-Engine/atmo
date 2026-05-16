#include "script.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"

namespace atmo::core::ecs::entities
{
    void Script::RegisterComponents(flecs::world *world)
    {
        world->component<components::ScriptTest>();
    }

    void Script::RegisterSystems(flecs::world *world)
    {
        world->system<components::ScriptTest>("Script_update").kind(flecs::OnValidate).each([](flecs::entity e, components::ScriptTest &script) {
            if (script.instance == nullptr) {
                return;
            }
            float dt = e.world().delta_time();

            script.instance->update(dt);
            script.instance->physiqueUpdate(dt);
        });

        world->observer<components::ScriptTest>("Script_remove").event(flecs::OnRemove).each([](flecs::entity e, components::ScriptTest &script) {
            if (script.script_path.empty())
                return;
            if (script.instance == nullptr) {
                return;
            }

            script.instance->destroy();
            script.m_res = nullptr;
        });
    }

    void Script::initialize()
    {
        Entity::initialize();

        setComponent<components::ScriptTest>({});
    }

    void Script::setScriptInstance(luau::ScriptInstance *instance)
    {
        auto script = p_handle.get_ref<components::ScriptTest>();
        script->instance = instance;
    }

    void Script::setScriptPath(const std::string &path)
    {
        auto script = p_handle.get_ref<components::ScriptTest>();
        script->script_path = path;

        if (script->script_path.empty())
            return;
        if (script->instance == nullptr) {
            return;
        }

        std::unique_ptr<resource::ResourceRef<resource::Bytecode>> res = resource::ResourceManager::GetInstance().getResource<resource::Bytecode>(script->script_path);

        script->m_res = std::move(res);

        spdlog::debug("Loaded script for entity {}: {}", p_handle.name().c_str(), script->script_path);

        script->instance->load("script test", script->m_res->get()->data, script->m_res->get()->size, p_handle.id());
        script->instance->create();
    }

    std::string_view Script::getScriptPath() const noexcept
    {
        auto script = p_handle.get_ref<components::ScriptTest>();
        return script->script_path;
    }

} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Script);
ATMO_REGISTER_COMPONENT(atmo::core::components::ScriptTest)
