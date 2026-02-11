#pragma once

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

                    std::string serialize() const override;
                    void deserialize(const std::string &data) override;

                    static constexpr std::string_view LocalName()
                    {
                        return "RectangleShape2d";
                    }

                private:
                    float m_radius{ 25.0f };
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo
