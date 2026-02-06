#include "circle_shape2d.hpp"
#include <format>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                std::string_view CircleShape2d::name() const noexcept
                {
                    return "Shape2d::CircleShape2d";
                }

                std::string CircleShape2d::serialize() const
                {
                    return std::format(R"({{ "radius": {} }})", m_radius);
                }

                void CircleShape2d::deserialize(const std::string &data) {}
            } // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo

namespace
{
    static int _ = [] {
        atmo::core::resource::SubResourceRegistry::RegisterType<atmo::core::resource::resources::CircleShape2d>("Shape2d::CircleShape2d");
        return 0;
    }();
} // namespace
