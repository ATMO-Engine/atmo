#include "schema.hpp"

#include <cmath>
#include <string_view>

namespace atmo::mcp
{
    namespace
    {
        constexpr std::string_view defs_prefix = "#/$defs/";
        constexpr double integer_limit = 2147483647.0;

        glz::generic Normalize(const glz::generic &node, const glz::generic::object_t &defs, bool *nullable)
        {
            if (!node.is_object())
                return node;

            glz::generic::object_t merged;
            const auto &object = node.get_object();
            bool optional_ref = false;
            if (auto ref = object.find("$ref"); ref != object.end() && ref->second.is_string()) {
                std::string_view name = ref->second.get_string();
                if (name.starts_with(defs_prefix))
                    name.remove_prefix(defs_prefix.size());
                optional_ref = name.starts_with("std::optional<");
                if (auto def = defs.find(name); def != defs.end() && def->second.is_object())
                    merged = def->second.get_object();
            }
            if (nullable && optional_ref)
                *nullable = true;
            for (const auto &[key, value] : object)
                if (key != "$ref")
                    merged[key] = value;

            glz::generic::object_t out;
            for (const auto &[key, value] : merged) {
                if (key == "$defs" || key == "$schema" || key == "title" || key == "required")
                    continue;

                if (key == "type" && value.is_array()) {
                    glz::generic::array_t types;
                    bool has_null = false;
                    for (const auto &type : value.get_array()) {
                        if (type.is_string() && type.get_string() == "null")
                            has_null = true;
                        else
                            types.push_back(type);
                    }
                    if (nullable && has_null && types.size() == 1)
                        *nullable = true;
                    if (types.size() == 1)
                        out["type"] = types.front();
                    else if (types.size() > 1 && types.size() < 5)
                        out["type"] = types;
                    continue;
                }

                if (key == "properties" && value.is_object()) {
                    glz::generic::object_t properties;
                    glz::generic::array_t required;
                    for (const auto &[property, schema] : value.get_object()) {
                        bool optional = false;
                        properties[property] = Normalize(schema, defs, &optional);
                        if (!optional)
                            required.emplace_back(property);
                    }
                    out["properties"] = std::move(properties);
                    if (!required.empty())
                        out["required"] = std::move(required);
                    continue;
                }

                if ((key == "items" || key == "additionalProperties") && value.is_object()) {
                    out[key] = Normalize(value, defs, nullptr);
                    continue;
                }

                if ((key == "oneOf" || key == "anyOf") && value.is_array()) {
                    glz::generic::array_t options;
                    for (const auto &option : value.get_array()) options.push_back(Normalize(option, defs, nullptr));
                    out[key] = std::move(options);
                    continue;
                }

                if ((key == "minimum" || key == "maximum") && value.is_number() && std::fabs(value.get_number()) >= integer_limit)
                    continue;

                out[key] = value;
            }
            return out;
        }
    } // namespace

    glz::generic NormalizeSchema(const glz::generic &glaze_schema)
    {
        glz::generic::object_t defs;
        if (glaze_schema.is_object()) {
            const auto &object = glaze_schema.get_object();
            if (auto it = object.find("$defs"); it != object.end() && it->second.is_object())
                defs = it->second.get_object();
        }
        return Normalize(glaze_schema, defs, nullptr);
    }
} // namespace atmo::mcp
