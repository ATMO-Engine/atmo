// #include "script.hpp"
// #include "core/resource/resource_manager.hpp"
// #include "core/resource/resource_ref.hpp"
//
// namespace atmo::core::ecs::entities
//{
//     void Script::RegisterSystems(flecs::world *world)
//     {
//         // S'exécute une seule fois par entité grâce à flecs::OnAdd
//         world->observer<components::ScriptTest>()
//         .event(flecs::OnAdd)
//         .each([&](flecs::entity e, components::ScriptTest &script) {
//             if (script->script_path.empty())
//                 return;
//             if (script->instance == nullptr) {
//                 return;
//             }
//
//             std::unique_ptr<resource::ResourceRef<resource::Bytecode>> res =
//                 resource::ResourceManager::GetInstance().getResource<resource::Bytecode>(script.script_path);
//
//             script.m_res = std::move(res);
//
//             spdlog::debug("Loaded script for entity {}: {}", p_handle.name().c_str(), script.script_path);
//
//             script.instance->load("script test", script.m_res->get()->data, script.m_res->get()->size, p_handle.id());
//             script.instance->create();
//         });
//
//         world->system<components::ScriptTest>("Script_update").kind(flecs::OnValidate).each([](flecs::entity e, components::ScriptTest &script) {
//             if (script.instance == nullptr) {
//                 return;
//             }
//             float dt = e.world().delta_time();
//
//             script.instance->update(dt);
//             script.instance->physiqueUpdate(dt);
//         });
//
//         world->observer<components::ScriptTest>("Script_remove").event(flecs::OnRemove).each([](flecs::entity e, components::ScriptTest &script) {
//             if (script.script_path.empty())
//                 return;
//             if (script.instance == nullptr) {
//                 return;
//             }
//
//             script.instance->destroy();
//             script.m_res = nullptr;
//         });
//     }
//
// } // namespace atmo::core::ecs::entities

// ATMO_REGISTER_ENTITY(entities::Script);
