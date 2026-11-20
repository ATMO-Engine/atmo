#pragma once

#include <string>

#include "glaze/glaze.hpp"

namespace atmo::mcp
{
    [[nodiscard]] glz::generic NormalizeSchema(const glz::generic &glaze_schema);

    template <class T> [[nodiscard]] glz::generic SchemaFor()
    {
        auto schema = glz::write_json_schema<T>();
        if (!schema)
            return glz::generic::object_t{ { "type", "object" } };
        auto parsed = glz::read_json<glz::generic>(*schema);
        if (!parsed)
            return glz::generic::object_t{ { "type", "object" } };
        glz::generic normalized = NormalizeSchema(*parsed);
        if (normalized.is_object())
            normalized["type"] = "object";
        return normalized;
    }
} // namespace atmo::mcp
