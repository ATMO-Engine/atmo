#include "camera_2d.hpp"

#include "core/ecs/entity_registry.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void Camera2d::RegisterSystems(flecs::world *world)
    {
        // Reset camera state at the start of each frame so stale state doesn't persist
        // when the camera is deactivated or destroyed.
        world->system("Camera2d_ResetWorldState")
            .kind(flecs::OnLoad)
            .run([](flecs::iter &it) { it.world().set<components::WorldCameraState>({ .has_camera = false }); });

        // Snapshot the active camera into a world singleton so rendering systems can
        // read it without running a per-entity query inside every render call.
        world->system<const components::CameraData, const components::Transform2d>("Camera2d_UpdateWorldState")
            .kind(flecs::PreUpdate)
            .each([](flecs::entity e, const components::CameraData &cam, const components::Transform2d &t) {
                if (!cam.is_active)
                    return;
                e.world().set<components::WorldCameraState>({ .has_camera = true, .zoom = cam.zoom, .position = t.position });
            });

        // Enforce the "one active camera per world" invariant on mutation.
        world->observer<components::CameraData>("Camera2d_EnforceUnique")
            .event(flecs::OnSet)
            .each([](flecs::entity e, components::CameraData &cam) {
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
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Camera2d);
ATMO_REGISTER_COMPONENT(atmo::core::components::CameraData);
