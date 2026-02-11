#include <format>

#include "circle_shape2d.hpp"
#include "core/resource/subresource_registry.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                std::string CircleShape2d::serialize() const
                {
                    return std::format(R"({{ "radius": {} }})", m_radius);
                }

                void CircleShape2d::deserialize(const std::string &data) {}
            } // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo

REGISTER_SUBRESOURCE(resources::CircleShape2d);
