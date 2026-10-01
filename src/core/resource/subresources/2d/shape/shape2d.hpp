#pragma once

#include "box2d/box2d.h"
#include "box2d/id.h"
#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/subresource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                class Shape2d : public SubResourceRegistry::Registrable<Shape2d, SubResource>
                {
                public:
                    using SubResourceRegistry::Registrable<Shape2d, SubResource>::Registrable;

                    static constexpr std::string_view LocalName()
                    {
                        return "Shape2d";
                    }

                    b2ShapeDef &getShapeDef();

                    virtual void create(b2BodyId body) = 0;
                    void destroy(bool defer_body_update = false);
                    bool isValid() const
                    {
                        return b2Shape_IsValid(p_shape_id);
                    }

                    bool isDirty() const
                    {
                        return p_dirty;
                    }
                    void clearDirty()
                    {
                        p_dirty = false;
                    }
                    void onFieldChanged() override
                    {
                        p_dirty = true;
                    }
                    void prepareForRemoval() override
                    {
                        destroy();
                    }

                protected:
                    b2ShapeId p_shape_id{ b2_nullShapeId };
                    b2ShapeDef p_shape_def{ b2DefaultShapeDef() };
                    bool p_dirty{ false };
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo
