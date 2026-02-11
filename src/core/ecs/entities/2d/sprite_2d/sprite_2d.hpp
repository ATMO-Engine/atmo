#pragma once

#include "core/ecs/entities/2d/entity_2d.hpp"

namespace atmo::core::ecs::entities
{
    class Sprite2d : public Entity2d
    {
    public:
        void initComponents() override;
    };
} // namespace atmo::core::ecs::entities
