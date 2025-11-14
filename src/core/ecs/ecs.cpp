#include "ecs.hpp"
#include "core/ecs/components.hpp"
#include "core/resource/resource_manager.hpp"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include "impl/window.hpp"

atmo::core::ecs::ECS::ECS() : m_world()
{
    reset();
}

void atmo::core::ecs::ECS::stop()
{
    m_world.quit();
}

void atmo::core::ecs::ECS::reset()
{
    m_world.reset();

    ECS_IMPORT(m_world, FlecsMeta);

    m_world.init_builtin_components();
    components::register_core_components(m_world);
    loadPrefabs();
}

void atmo::core::ecs::ECS::loadPrefabs()
{
    m_world.system<components::Transform2D, components::Transform2D>("Transform2D_GenerateGlobal")
        .kind(flecs::PreUpdate)
        .term_at(1)
        .cascade(flecs::ChildOf)
        .each([](flecs::entity e, components::Transform2D &t, const components::Transform2D &parent_t) {
            t.g_position = { parent_t.g_position.x + t.position.x, parent_t.g_position.y + t.position.y };
            t.g_rotation = parent_t.g_rotation + t.rotation;
            t.g_scale = { parent_t.g_scale.x * t.scale.x, parent_t.g_scale.y * t.scale.y };
        });

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
        auto sprite2DPrefab = Prefab(m_world, "sprite2d").set<components::Transform2D>({}).set<components::Sprite2D>({ "" });

        m_world.system<components::Sprite2D>("Sprite2D_LoadTexture").kind(flecs::OnSet).each([](flecs::entity e, components::Sprite2D &sprite) {
            sprite.m_handle = atmo::core::resource::ResourceManager::getInstance().generate(sprite.texture_path);

            auto res = atmo::core::resource::ResourceManager::getInstance().getResource(sprite.m_handle);
            auto surface = std::any_cast<SDL_Surface *>(res->get());
            sprite.texture_size = { static_cast<float>(surface->w), static_cast<float>(surface->h) };
        });

        m_world.system<components::Sprite2D, components::Transform2D>("Sprite2D_UpdateDestRect")
            .kind(flecs::OnValidate)
            .each([](flecs::entity e, components::Sprite2D &sprite, components::Transform2D &transform) {
                sprite.m_dest_rect.x = transform.g_position.x;
                sprite.m_dest_rect.y = transform.g_position.y;
                sprite.m_dest_rect.w = sprite.texture_size.x * transform.g_scale.x;
                sprite.m_dest_rect.h = sprite.texture_size.y * transform.g_scale.y;
            });

        m_world.system<components::Sprite2D, components::Transform2D, core::ComponentManager::Managed, core::components::Window>("Sprite2D_Render")
            .kind(flecs::OnValidate)
            .term_at(3)
            .cascade(flecs::ChildOf)
            .each([](flecs::entity e,
                     components::Sprite2D &sprite,
                     components::Transform2D &transform,
                     core::ComponentManager::Managed &manager,
                     core::components::Window &window) {
                auto wm = static_cast<impl::WindowManager *>(manager.ptr);

                if (!sprite.m_handle.frameToLive)
                    return;

                SDL_Texture *texture = wm->getTextureFromHandle(sprite.m_handle);
                if (!texture)
                    return;

                SDL_RenderTexture(wm->getRenderer(), texture, nullptr, &sprite.m_dest_rect);
            });

        addPrefab(sprite2DPrefab);
    }
}

void atmo::core::ecs::ECS::addPrefab(Prefab &prefab)
{
    m_prefabs.emplace(prefab.name, prefab);
}

atmo::core::ecs::Entity atmo::core::ecs::ECS::instantiatePrefab(const std::string &name, const std::string &instance_name)
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
