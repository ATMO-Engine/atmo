#pragma once

#include <string>
#include <string_view>
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"

namespace atmo::editor
{
    class Editor
    {
    public:
        virtual ~Editor() = default;

        static constexpr std::string_view FullName()
        {
            return "Editor";
        }


        virtual std::string_view name() = 0;
        virtual std::string_view description() = 0;
        virtual std::string_view iconPath() = 0;

        virtual void init(atmo::core::ecs::entities::UI &container) = 0;
    };
} // namespace atmo::editor
