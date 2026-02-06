#include "rectangle_shape2d.hpp"
#include <format>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                std::string_view RectangleShape2d::name() const noexcept
                {
                    return "Shape2d::RectangleShape2d";
                }

                std::string RectangleShape2d::serialize() const
                {
                    return std::format(R"({{ "size": {{ "x": {}, "y": {} }} }})", m_size.x, m_size.y);
                }

                void RectangleShape2d::deserialize(const std::string &data) {}
            } // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo

namespace
{
    static int _ = [] {
        atmo::core::resource::SubResourceRegistry::RegisterType<atmo::core::resource::resources::RectangleShape2d>("Shape2d::RectangleShape2d");
        return 0;
    }();
} // namespace
