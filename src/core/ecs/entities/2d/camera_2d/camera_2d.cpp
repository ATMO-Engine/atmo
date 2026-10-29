#include "camera_2d.hpp"

#include "canvas_view.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "core/resource/resource_manager.hpp"
#include "meta/auto_register.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::core::ecs::entities
{
    namespace
    {
        void LoadPostProcess(components::CameraData &cam)
        {
            cam.m_loaded_post_process = cam.post_process;
            cam.m_post_materials.clear();
            for (const std::string &path : cam.post_process) {
                if (path.empty())
                    continue;
                try {
                    auto material = resource::ResourceManager::GetInstance().getResource<resource::Material>(path);
                    if (material && material->kind() != server::rendering::ShaderKind::Viewport) {
                        spdlog::warn("Camera post-process material {} needs an atmo::IViewportMaterial shader", path);
                        continue;
                    }
                    cam.m_post_materials.push_back(std::move(material));
                } catch (const std::exception &e) {
                    spdlog::error("Failed to open post-process material: {}", e.what());
                }
            }
        }
    } // namespace

    void Camera2d::RegisterSystems(flecs::world *world)
    {
        world->component<components::WorldCameraState>();
        world->component<components::WorldPostProcessState>();

        world->system("Camera2d_ResetWorldState").kind(flecs::OnLoad).run([](flecs::iter &it) {
            it.world().set<components::WorldCameraState>({ .has_camera = false });
        });

        world->system<components::CameraData, const components::Transform2d>("Camera2d_UpdateWorldState")
            .kind(flecs::PreUpdate)
            .each([](flecs::entity e, components::CameraData &cam, const components::Transform2d &t) {
                if (cam.post_process != cam.m_loaded_post_process)
                    LoadPostProcess(cam);
                if (!cam.is_active)
                    return;
                std::vector<server::rendering::MaterialRid> post;
                for (const auto &material : cam.m_post_materials)
                    if (server::rendering::MaterialRid rid = material ? material->rid() : server::rendering::MaterialRid{}; rid.isValid())
                        post.push_back(rid);
                e.world().set<components::WorldCameraState>({ .has_camera = true, .zoom = cam.zoom, .position = t.position, .post_process = std::move(post) });
            });

        world->system("Camera2d_ApplyView").kind(flecs::OnValidate).run([](flecs::iter &it) {
            const auto *ctx = it.world().try_get<components::WorldContext>();
            if (!ctx || !ctx->viewport.isValid())
                return;

            const auto *camera_state = it.world().try_get<components::WorldCameraState>();
            const CameraView camera{
                .active = camera_state && camera_state->has_camera,
                .zoom = camera_state ? camera_state->zoom : 1.0f,
                .position = camera_state ? camera_state->position : types::Vector2{ 0.0f, 0.0f },
            };

            StretchMode mode = StretchMode::Disabled;
            if (!ctx->is_editor_isolated) {
                const auto &window = project::ProjectManager::GetSettings().window;
                if (std::optional<StretchMode> parsed = ParseStretchMode(window.stretch_mode)) {
                    mode = *parsed;
                } else {
                    static bool warned = false;
                    if (!warned)
                        spdlog::warn("Unknown window.stretch_mode '{}', expected disabled, fit or integer", window.stretch_mode);
                    warned = true;
                }
            }

            const auto &design = project::ProjectManager::GetSettings().window.size;
            const CanvasView view =
                ComputeCanvasView(mode, { static_cast<float>(design.x), static_cast<float>(design.y) }, ctx->viewport_size, ctx->viewport_dpi, camera);

            auto &server = server::rendering::RenderingServer::Instance();
            server.viewportSetCanvasTransform(
                ctx->viewport, ctx->is_editor_isolated ? server::rendering::Affine2d::Scaling(ctx->viewport_dpi) * view.transform : view.transform);
            server.viewportSetContentRect(ctx->viewport, view.content);

            const std::vector<server::rendering::MaterialRid> post = camera_state ? camera_state->post_process : std::vector<server::rendering::MaterialRid>{};
            const auto *applied = it.world().try_get<components::WorldPostProcessState>();
            if (!applied || applied->applied != post || applied->viewport != ctx->viewport) {
                server.viewportSetPostProcess(ctx->viewport, post);
                it.world().set<components::WorldPostProcessState>({ .applied = post, .viewport = ctx->viewport });
            }
        });

        world->observer<components::CameraData>("Camera2d_EnforceUnique").event(flecs::OnSet).each([](flecs::entity e, components::CameraData &cam) {
            if (!cam.is_active)
                return;
            e.world().each<components::CameraData>([&](flecs::entity other, components::CameraData &other_cam) {
                if (other != e && other_cam.is_active)
                    other_cam.is_active = false;
            });
        });
    }

    void Camera2d::initialize()
    {
        Entity2d::initialize();
        setComponent<components::CameraData>({});
    }

    void Camera2d::setZoom(float z)
    {
        auto cam = p_handle.get_ref<components::CameraData>();
        cam->zoom = z;
    }

    float Camera2d::getZoom() const
    {
        auto cam = p_handle.get_ref<components::CameraData>();
        return cam->zoom;
    }

    void Camera2d::setActive(bool active)
    {
        auto cam = p_handle.get_ref<components::CameraData>();
        cam->is_active = active;
    }

    bool Camera2d::isActive() const
    {
        auto cam = p_handle.get_ref<components::CameraData>();
        return cam->is_active;
    }

    void Camera2d::setPostProcess(const std::vector<std::string> &materials)
    {
        auto cam = p_handle.get_ref<components::CameraData>();
        cam->post_process = materials;
    }

    const std::vector<std::string> &Camera2d::getPostProcess() const
    {
        auto cam = p_handle.get_ref<components::CameraData>();
        return cam->post_process;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Camera2d);
ATMO_REGISTER_COMPONENT(atmo::core::components::CameraData);
