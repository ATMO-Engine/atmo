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
                class RectangleShape2d : public SubResourceRegistry::Registrable<RectangleShape2d, Shape2d>
                {
                public:
                    using SubResourceRegistry::Registrable<RectangleShape2d, Shape2d>::Registrable;

                    friend struct atmo::meta::ComponentMeta<RectangleShape2d>;

                    std::string serialize() const override;
                    void deserialize(const std::string &data) override;

                    static constexpr std::string_view LocalName()
                    {
                        return "RectangleShape2d";
                    }

                    std::string_view getTypeName() const override
                    {
                        return FullName();
                    }

                    void setSize(const types::Vector2 &size);
                    types::Vector2 getSize() const;

                    void create(b2BodyId body) override;

                private:
                    types::Vector2 m_size{ 50.0f, 50.0f };
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo

template <> struct atmo::meta::ComponentMeta<atmo::core::resource::resources::RectangleShape2d> {
    static constexpr const char *name = "SubResource::Shape2d::RectangleShape2d";
    static constexpr auto fields =
        std::make_tuple(atmo::meta::field<&atmo::core::resource::resources::RectangleShape2d::m_size>("size"));
};
