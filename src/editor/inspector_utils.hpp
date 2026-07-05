#pragma once

#include <vector>

#include "core/ecs/entities/entity.hpp"
#include "meta/type_info.hpp"

namespace atmo::editor
{
    struct FieldWidgetRow {
        core::ecs::entities::Entity widget;
        meta::FieldInfo field;
    };

    /**
     * @brief Builds one label+widget row per field in `ti->fields`, parented under `container`, using `data` as the
     * raw pointer the widgets read/write through (a flecs component pointer, or a raw subresource pointer for
     * inline nested rendering). Returns the created rows so the caller can decide how to wire up live per-frame
     * updates (or skip that entirely, e.g. for subresource elements that nothing else mutates).
     */
    std::vector<FieldWidgetRow> buildFieldWidgetRows(const meta::TypeInfo *ti, void *data, core::ecs::entities::Entity container);
} // namespace atmo::editor
