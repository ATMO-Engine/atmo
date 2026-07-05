#include "core/resource/subresource_registry.hpp"
#include "glaze/glaze.hpp"
#include "meta/auto_register.hpp"
#include "rectangle_shape2d.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::resource::resources
{
    std::string RectangleShape2d::serialize() const
    {
        return glz::write_json(*this).value_or("{}");
    }

    void RectangleShape2d::deserialize(const std::string &data)
    {
        auto err = glz::read_json(*this, data);
        if (err) {
            spdlog::error("Failed to deserialize RectangleShape2d: {}", glz::format_error(err, data));
        }
    }

    void RectangleShape2d::setSize(const types::Vector2 &size)
    {
        m_size = size;
    }

    types::Vector2 RectangleShape2d::getSize() const
    {
        return m_size;
    }

    void RectangleShape2d::create(b2BodyId body)
    {
        auto halfSize = m_size / 2;
        b2Vec2 meterVector = halfSize;
        b2Polygon box = b2MakeBox(meterVector.x, meterVector.y);

        p_shape_id = b2CreatePolygonShape(body, &p_shape_def, &box);
    }
} // namespace atmo::core::resource::resources

REGISTER_SUBRESOURCE(resources::RectangleShape2d);
ATMO_REGISTER_COMPONENT_NO_FLECS(atmo::core::resource::resources::RectangleShape2d)
