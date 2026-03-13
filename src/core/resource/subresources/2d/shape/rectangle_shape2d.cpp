#include <format>

#include "core/resource/subresource_registry.hpp"
#include "rectangle_shape2d.hpp"

namespace atmo::core::resource::resources
{
    std::string RectangleShape2d::serialize() const
    {
        return std::format(R"({{ "size": {{ "x": {}, "y": {} }} }})", m_size.x, m_size.y);
    }

    void RectangleShape2d::deserialize(const std::string &data) {}

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
