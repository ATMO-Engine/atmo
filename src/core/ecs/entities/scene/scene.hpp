#pragma once

#include "box2d/types.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    class Scene : public EntityRegistry::Registrable<Scene, Entity>
    {
    public:
        using EntityRegistry::Registrable<Scene, Entity>::Registrable;

        static void RegisterComponents(flecs::world *world);
        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Scene";
        }

        /**
         * @brief Initialize the scene from a file.
         *
         * The file should be a JSON file.
         *
         * @param file_path Path to the scene file.
         */
        void initFromFile(std::string_view file_path);

        void setSingleton(bool singleton);

        b2WorldId getWorldId() const
        {
            auto scene = p_handle.get_ref<components::Scene>();
            return scene->world_id;
        }

    private:
        static void SetupDebugDraw(b2DebugDraw *debugDraw);

        static b2DebugDraw m_debug_draw;
    };
} // namespace atmo::core::ecs::entities
