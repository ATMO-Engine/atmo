#include "shape2d.hpp"
#include "box2d/box2d.h"
#include "core/resource/subresource_registry.hpp"

namespace atmo::core::resource::resources
{
    b2ShapeDef &Shape2d::getShapeDef()
    {
        return p_shape_def;
    }

    void Shape2d::destroy(bool defer_body_update)
    {
        if (b2Shape_IsValid(p_shape_id))
            b2DestroyShape(p_shape_id, !defer_body_update);
    }
} // namespace atmo::core::resource::resources

REGISTER_SUBRESOURCE(resources::Shape2d);
