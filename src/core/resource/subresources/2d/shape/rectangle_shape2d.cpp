#include <format>

#include "core/resource/subresource_registry.hpp"
#include "rectangle_shape2d.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                std::string RectangleShape2d::serialize() const
                {
                    return std::format(R"({{ "size": {{ "x": {}, "y": {} }} }})", m_size.x, m_size.y);
                }

                void RectangleShape2d::deserialize(const std::string &data) {}
            } // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo

REGISTER_SUBRESOURCE(resources::RectangleShape2d);
