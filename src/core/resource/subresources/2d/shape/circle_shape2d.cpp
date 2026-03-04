#include <format>

#include "box2d/box2d.h"
#include "circle_shape2d.hpp"
#include "core/resource/subresource_registry.hpp"

namespace atmo::core::resource::resources
{
    std::string CircleShape2d::serialize() const
    {
        return std::format(R"({{ "radius": {} }})", m_radius);
    }

    void CircleShape2d::deserialize(const std::string &data) {}

    void CircleShape2d::setRadius(float radius)
    {
        m_radius = radius;
    }

    float CircleShape2d::getRadius() const
    {
        return m_radius;
    }

    void CircleShape2d::create(b2BodyId body)
    {
        b2Circle circle = { .center = { 0, 0 }, .radius = m_radius };

        p_shape_id = b2CreateCircleShape(body, &p_shape_def, &circle);
    }
} // namespace atmo::core::resource::resources

REGISTER_SUBRESOURCE(resources::CircleShape2d);
