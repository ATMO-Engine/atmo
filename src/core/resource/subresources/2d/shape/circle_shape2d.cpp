#include "box2d/box2d.h"
#include "circle_shape2d.hpp"
#include "common/math.hpp"
#include "core/resource/subresource_registry.hpp"
#include "glaze/glaze.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::resource::resources
{
    std::string CircleShape2d::serialize() const
    {
        return glz::write_json(*this).value_or("{}");
    }

    void CircleShape2d::deserialize(const std::string &data)
    {
        auto err = glz::read_json(*this, data);
        if (err) {
            spdlog::error("Failed to deserialize CircleShape2d: {}", glz::format_error(err, data));
        }
    }

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
        b2Circle circle = { .center = { 0, 0 }, .radius = common::math::PixelToMeter(m_radius) };

        p_shape_id = b2CreateCircleShape(body, &p_shape_def, &circle);
    }
} // namespace atmo::core::resource::resources

REGISTER_SUBRESOURCE(resources::CircleShape2d);
ATMO_REGISTER_COMPONENT_NO_FLECS(atmo::core::resource::resources::CircleShape2d)
