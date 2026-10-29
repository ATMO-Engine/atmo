#pragma once

#include <memory>
#include <string>
#include <vector>
#include "core/ecs/entities/2d/entity_2d.hpp"

#include "core/ecs/entity_registry.hpp"
#include "core/resource/material.hpp"
#include "core/types.hpp"
#include "meta/component_meta.hpp"
#include "meta/field_descriptor.hpp"

namespace atmo::core::components
{
    struct CameraData {
        float zoom = 1.0f;
        bool is_active = true;
        std::vector<std::string> post_process;
        std::vector<std::string> m_loaded_post_process;
        std::vector<std::shared_ptr<resource::Material>> m_post_materials;
    };

    struct WorldCameraState {
        bool has_camera = false;
        float zoom = 1.0f;
        types::Vector2 position{ 0.0f, 0.0f };
        std::vector<server::rendering::MaterialRid> post_process;
    };

    struct WorldPostProcessState {
        std::vector<server::rendering::MaterialRid> applied;
        server::rendering::ViewportRid viewport;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::CameraData> {
    static constexpr const char *name = "CameraData";
    static constexpr const char *category = "2D";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::CameraData::zoom>("zoom"), atmo::meta::field<&atmo::core::components::CameraData::is_active>("is_active"),
        atmo::meta::field<&atmo::core::components::CameraData::post_process>("post_process").skipFlecs());
};

namespace atmo::core::ecs::entities
{
    class Camera2d : public EntityRegistry::Registrable<Camera2d, Entity2d>
    {
    public:
        using EntityRegistry::Registrable<Camera2d, Entity2d>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "Camera2d";
        }

        static void RegisterSystems(flecs::world *world);

        void initialize();

        void setZoom(float z);
        float getZoom() const;

        void setActive(bool active);
        bool isActive() const;

        void setPostProcess(const std::vector<std::string> &materials);
        const std::vector<std::string> &getPostProcess() const;
    };
} // namespace atmo::core::ecs::entities
