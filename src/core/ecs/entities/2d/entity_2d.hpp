#pragma once

#include <memory>
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "flecs.h"

namespace atmo::core::ecs::entities
{
    class Entity2d : public EntityRegistry::Registrable<Entity2d, Entity>
    {
    public:
        using EntityRegistry::Registrable<Entity2d, Entity>::Registrable;

        static void RegisterComponents(flecs::world *world);
        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Entity2d";
        }

        /**
         * @brief Get the Position of the entity.
         *
         * @return types::Vector2 Position of the entity.
         */
        types::Vector2 getPosition() const;

        /**
         * @brief Set the Position of the entity.
         *
         * @param position New position for the entity.
         */
        void setPosition(const types::Vector2 &position);

        /**
         * @brief Get the Global Position of the entity, which is the position of the entity in world space, taking into account the positions of all parent
         * entities.
         *
         * @return types::Vector2 Global Position of the entity.
         */
        types::Vector2 getGlobalPosition() const;

        /**
         * @brief Get the Scale of the entity.
         *
         * @return types::Vector2 Scale of the entity.
         */
        types::Vector2 getScale() const;

        /**
         * @brief Set the Scale of the entity.
         *
         * @param scale New scale for the entity.
         */
        void setScale(const types::Vector2 &scale);

        /**
         * @brief Get the Global Scale of the entity, which is the scale of the entity in world space, taking into account the scales of all parent entities.
         *
         * @return types::Vector2 Global Scale of the entity.
         */
        types::Vector2 getGlobalScale() const;

        /**
         * @brief Get the Rotation of the entity in degrees.
         *
         * @return float Rotation of the entity in degrees.
         */
        float getRotation() const;

        /**
         * @brief Set the Rotation of the entity in degrees.
         *
         * @param rotation New rotation for the entity in degrees.
         */
        void setRotation(float rotation);

        /**
         * @brief Get the Global Rotation of the entity in degrees, which is the rotation of the entity in world space, taking into account the rotations of all
         * parent entities.
         *
         * @return float Global Rotation of the entity in degrees.
         */
        float getGlobalRotation() const;

        /**
         * @brief Get the Scene that the entity belongs to.
         *
         * @return std::shared_ptr<entities::Scene> Scene that the entity belongs to.
         */
        std::shared_ptr<entities::Scene> getScene() const;

    private:
        static flecs::query<> p_SceneQuery;
    };
} // namespace atmo::core::ecs::entities
