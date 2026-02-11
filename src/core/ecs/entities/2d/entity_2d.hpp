#pragma once

#include "core/ecs/entities/entity.hpp"

namespace atmo::core::ecs::entities
{
    class Entity2d : public Entity
    {
    public:
        void initComponents() override;
    };
} // namespace atmo::core::ecs::entities
