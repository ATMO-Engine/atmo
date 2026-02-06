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
                class CircleShape2d : public Shape2d
                {
                public:
                    std::string_view name() const noexcept override;

                    std::string serialize() const override;
                    void deserialize(const std::string &data) override;

                private:
                    float m_radius{ 25.0f };
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo
