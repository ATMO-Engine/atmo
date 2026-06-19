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


        static std::string_view Name()
        {
            throw std::logic_error("Name() not implemented");
        }

        static std::string_view Description()
        {
            throw std::logic_error("Description() not implemented");
        }

        static std::string_view IconPath()
        {
            throw std::logic_error("IconPath() not implemented");
        }

        virtual void init(atmo::core::ecs::entities::UI &container) = 0;
    };
} // namespace atmo::editor
