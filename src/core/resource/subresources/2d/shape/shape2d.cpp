#include "shape2d.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                std::string_view Shape2d::name() const noexcept
                {
                    return "Shape2d";
                }
            } // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo

namespace
{
    static int _ = [] {
        atmo::core::resource::SubResourceRegistry::RegisterType<atmo::core::resource::resources::Shape2d>("Shape2d");
        return 0;
    }();
} // namespace
