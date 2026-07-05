#pragma once

#include "meta/meta.hpp"
#include "shape2d.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            namespace resources
            {
                class CircleShape2d : public SubResourceRegistry::Registrable<CircleShape2d, Shape2d>
                {
                public:
                    using SubResourceRegistry::Registrable<CircleShape2d, Shape2d>::Registrable;

                    friend struct atmo::meta::ComponentMeta<CircleShape2d>;

                    std::string serialize() const override;
                    void deserialize(const std::string &data) override;

                    static constexpr std::string_view LocalName()
                    {
                        return "CircleShape2d";
                    }

                    std::string_view getTypeName() const override
                    {
                        return FullName();
                    }

                    void setRadius(float radius);
                    float getRadius() const;

                    void create(b2BodyId body) override;

                private:
                    float m_radius{ 2.0f };
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo

template <> struct atmo::meta::ComponentMeta<atmo::core::resource::resources::CircleShape2d> {
    static constexpr const char *name = "SubResource::Shape2d::CircleShape2d";
    static constexpr auto fields =
        std::make_tuple(atmo::meta::field<&atmo::core::resource::resources::CircleShape2d::m_radius>("radius"));
};
