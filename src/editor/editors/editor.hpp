#pragma once

#include "string"
#include "string_view"

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

        virtual std::string serialize() const = 0;
        virtual void deserialize(const std::string &data) = 0;
    };
} // namespace atmo::editor
