#include "ecs.hpp"
#include <memory>
#include "SDL3/SDL_render.h"
#include "core/ecs/components.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/scene/scene_manager.hpp"
#include "impl/window.hpp"

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            ECS::ECS()
            {
                reset();
                m_scene_manager.setWorld(m_world);
            }

            void ECS::stop()
            {
                m_world.quit();
            }

            void ECS::reset()
            {
                m_world.reset();

                // ECS_IMPORT(m_world, FlecsMeta);

#if defined(ATMO_DEBUG)
                m_world.import <flecs::stats>();
                m_world.set<flecs::Rest>({});
#endif

                m_world.init_builtin_components();
                components::register_core_components(m_world);
                loadPrefabs();
            }

            void ECS::loadPrefabs()
            {
                m_world.system<components::Transform2D, components::Transform2D>("Transform2D_GenerateGlobal")
                    .kind(flecs::PreUpdate)
                    .term_at(1)
                    .up()
                    .each([](flecs::entity e, components::Transform2D &t, const components::Transform2D &parent_t) {
                        t.g_position = { parent_t.g_position.x + t.position.x, parent_t.g_position.y + t.position.y };
                        t.g_rotation = parent_t.g_rotation + t.rotation;
                        t.g_scale = { parent_t.g_scale.x * t.scale.x, parent_t.g_scale.y * t.scale.y };
                    });

                { // Scene
                    auto scenePrefab = Prefab(m_world, "scene").set(components::Scene{ false });

                    addPrefab(scenePrefab);
                }

                { // Window
                    auto windowPrefab = Prefab(m_world, "window").managed<impl::WindowManager>(components::Window{ "Atmo Managed Window", { 800, 600 } });

                    m_world.system<core::ComponentManager::Managed, core::components::Window>("PollEvents")
                        .kind(flecs::PreUpdate)
                        .each([](flecs::iter &it, size_t i, core::ComponentManager::Managed &manager, core::components::Window &window) {
                            auto wm = static_cast<impl::WindowManager *>(manager.ptr);
                            wm->pollEvents(it.delta_time());
                            wm->beginDraw();
                        });

                    m_world.system<core::ComponentManager::Managed, core::components::Window>("Draw")
                        .kind(flecs::PostUpdate)
                        .each([](core::ComponentManager::Managed &manager, core::components::Window &window) {
                            auto wm = static_cast<impl::WindowManager *>(manager.ptr);
                            wm->draw();
                        });

                    addPrefab(windowPrefab);
                }

                { // Sprite2D
                    m_world.observer<components::Sprite2D>("Sprite2D_LoadTexture").event(flecs::OnSet).each([](flecs::entity e, components::Sprite2D &sprite) {
                        if (sprite.texture_path.empty())
                            return;

                        sprite.m_handle = atmo::core::resource::Handle<SDL_Surface>{ .assetId = sprite.texture_path };

                        atmo::core::resource::ResourceRef<SDL_Surface> res =
                            atmo::core::resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite.m_handle.assetId);

                        res.pin();

                        spdlog::debug("Loaded Sprite2D texture for entity {}: {}", e.name().c_str(), sprite.texture_path);

                        std::shared_ptr<SDL_Surface> surface = res.get();

                        spdlog::debug("Sprite2D texture size: {}x{}", surface->w, surface->h);

                        sprite.texture_size = { static_cast<float>(surface->w), static_cast<float>(surface->h) };
                    });

                    m_world.system<components::Sprite2D, components::Transform2D>("Sprite2D_UpdateDestRect")
                        .kind(flecs::OnValidate)
                        .each([](flecs::entity e, components::Sprite2D &sprite, components::Transform2D &transform) {
                            sprite.m_dest_rect.x = transform.g_position.x + transform.position.x;
                            sprite.m_dest_rect.y = transform.g_position.y + transform.position.y;
                            sprite.m_dest_rect.w = sprite.texture_size.x * (transform.g_scale.x * transform.scale.x);
                            sprite.m_dest_rect.h = sprite.texture_size.y * (transform.g_scale.y * transform.scale.y);
                        });

                    m_world.system<components::Sprite2D, components::Transform2D, ComponentManager::Managed, components::Window>("Sprite2D_Render")
                        .kind(flecs::OnValidate)
                        .term_at(2)
                        .up()
                        .term_at(3)
                        .up()
                        .each([](flecs::entity e,
                                 components::Sprite2D &sprite,
                                 components::Transform2D &transform,
                                 ComponentManager::Managed &manager,
                                 core::components::Window &window) {
                            auto wm = static_cast<impl::WindowManager *>(manager.ptr);

                            SDL_Texture *texture = wm->getTextureFromHandle(sprite.m_handle);
                            if (!texture)
                                return;

                            SDL_RenderTextureRotated(
                                wm->getRenderer(), texture, nullptr, &sprite.m_dest_rect, transform.g_rotation + transform.rotation, nullptr, SDL_FLIP_NONE);
                        });

                    auto sprite2DPrefab = Prefab(m_world, "sprite2d").set(components::Transform2D{}).set(components::Sprite2D{});
                    addPrefab(sprite2DPrefab);

                    m_world.observer<components::Sprite2D>("Sprite2D_remove").event(flecs::OnRemove).each([](flecs::entity e, components::Sprite2D &sprite) {
                        if (sprite.texture_path.empty())
                            return;

                        atmo::core::resource::ResourceRef<SDL_Surface> res =
                            atmo::core::resource::ResourceManager::GetInstance().getResource<SDL_Surface>(sprite.m_handle.assetId);

                        res.unpin();

                        spdlog::debug("Unpinned Sprite2D texture for entity {}: {}", e.name().c_str(), sprite.texture_path);
                    });
                }
            }

            flecs::entity ECS::createScene(const std::string &scene_name, bool singleton)
            {
                flecs::entity scene = instantiatePrefab("scene", scene_name).set<components::Scene>({ singleton });

                return scene;
            }

            void ECS::changeScene(flecs::entity scene)
            {
                flecs::entity current = m_scene_manager.getCurrentScene();

                if (current && scene == current)
                    return;

                try {
                    m_scene_manager.changeScene(scene);
                } catch (const scene::SceneManager::SwitchToSingletonSceneException &e) {
                    return;
                }
            }

            void ECS::changeSceneToFile(std::string_view scene_path)
            {
                m_scene_manager.changeSceneToFile(scene_path);
            }

            void ECS::addPrefab(Prefab &prefab)
            {
                m_prefabs.emplace(prefab.name, prefab);
            }

            Entity ECS::instantiatePrefab(const std::string &name, const std::string &instance_name)
            {
                if (m_prefabs.find(name) == m_prefabs.end()) {
                    throw std::runtime_error("Prefab not found: " + name);
                }

                flecs::entity instance;
                if (instance_name.empty()) {
                    instance = m_world.entity().is_a(m_prefabs.at(name).entity);
                } else {
                    instance = m_world.entity(instance_name.c_str()).is_a(m_prefabs.at(name).entity);
                }

                return instance;
            }
        } // namespace ecs
    } // namespace core

} // namespace atmo
