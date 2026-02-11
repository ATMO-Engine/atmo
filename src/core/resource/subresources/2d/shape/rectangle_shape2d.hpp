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
                class RectangleShape2d : public Shape2d
                {
                public:
                    std::string serialize() const override;
                    void deserialize(const std::string &data) override;

                private:
                    types::Vector2 m_size{ 50.0f, 50.0f };
                };
            }; // namespace resources
        } // namespace resource
    } // namespace core
} // namespace atmo
