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
                class RectangleShape2d : public SubResourceRegistry::Registrable<RectangleShape2d, Shape2d>
                {
                public:
                    using SubResourceRegistry::Registrable<RectangleShape2d, Shape2d>::Registrable;

                    std::string serialize() const override;
                    void deserialize(const std::string &data) override;

                    static constexpr std::string_view LocalName()
                    {
                        return "RectangleShape2d";
                    }

                private:
                    types::Vector2 m_size{ 50.0f, 50.0f };
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo
