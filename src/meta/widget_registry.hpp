#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "core/ecs/entities/entity.hpp"
#include "meta/type_info.hpp"
#include "spdlog/spdlog.h"

#define ATMO_REGISTER_WIDGET(name, create, destroy, update)                       \
    namespace                                                                     \
    {                                                                             \
        static int _ = [] {                                                       \
            using namespace atmo::meta;                                           \
            WidgetRegistry::get().register_widget(name, create, destroy, update); \
            return 0;                                                             \
        }();                                                                      \
    }

namespace atmo::meta
{
    class WidgetRegistry
    {
    public:
        using CreateFn = std::function<std::optional<core::ecs::entities::Entity>(core::ecs::entities::Entity parent, void *value, const FieldInfo &field)>;
        using DestroyFn = std::function<void(core::ecs::entities::Entity widget)>;
        using UpdateFn = std::function<void(core::ecs::entities::Entity widget, void *value, const FieldInfo &field)>;

        struct WidgetHandler {
            CreateFn create;
            DestroyFn destroy;
            UpdateFn update;
        };

        static WidgetRegistry &get()
        {
            static WidgetRegistry instance;
            return instance;
        }

        WidgetRegistry(const WidgetRegistry &) = delete;
        WidgetRegistry &operator=(const WidgetRegistry &) = delete;

        void register_widget(std::string_view name, CreateFn create, DestroyFn destroy, UpdateFn update)
        {
            m_widgets[std::string(name)] = WidgetHandler{ std::move(create), std::move(destroy), std::move(update) };
        }

        /**
         * @brief Create a widget entity for the given field and attach it to the parent entity.
         *
         * @param parent Parent entity to attach the widget to.
         * @param value Pointer to the component instance that the widget will edit. The widget is responsible for casting this to the correct type and writing
         * back any changes.
         * @param field FieldInfo describing the field that the widget is editing. Contains metadata such as label, tooltip, and widget type.
         * @return std::optional<core::ecs::entities::Entity> The created widget entity, or std::nullopt if creation failed (e.g. no handler registered for
         * field.widget).
         */
        [[nodiscard]] std::optional<core::ecs::entities::Entity> create(core::ecs::entities::Entity parent, void *value, const FieldInfo &field) const
        {
            if (!field.widget) {
                spdlog::error(R"(Couldn't create widget for field of type "{}": widget not configured for field)", field.widget);
                return std::nullopt;
            }

            auto it = m_widgets.find(field.widget);
            if (it == m_widgets.end()) {
                spdlog::error(R"(Couldn't create widget for field of type "{}": widget not found in registry for type)", field.widget);
                return std::nullopt;
            }
            auto widget = it->second.create(parent, value, field);

            if (widget) {
                widget->setParent(parent);
                return widget;
            }
            return std::nullopt;
        }

        /**
         * @brief Destroy a widget entity that was created for a field. Should be called when the widget is no longer needed (e.g. when the editor UI is
         * closed or the field's component instance is destroyed) to allow for proper cleanup.
         *
         * @param widget The widget entity to destroy.
         * @param field FieldInfo describing the field that the widget was editing. Used to look up the correct destroy handler. If field.widget is nullptr
         * or has no
         */
        void destroy(core::ecs::entities::Entity widget, const FieldInfo &field) const
        {
            if (!field.widget) {
                return;
            }

            auto it = m_widgets.find(field.widget);
            if (it == m_widgets.end()) {
                return;
            }

            it->second.destroy(widget);
        }


        void update(core::ecs::entities::Entity widget, void *value, const FieldInfo &field) const
        {
            if (widget.isAlive() && !field.widget) {
                return;
            }

            auto it = m_widgets.find(field.widget);
            if (it == m_widgets.end()) {
                return;
            }

            it->second.update(widget, value, field);
        }

        [[nodiscard]] bool hasWidget(std::string_view name) const
        {
            return m_widgets.contains(std::string(name));
        }

    private:
        WidgetRegistry() = default;

        std::unordered_map<std::string, WidgetHandler> m_widgets;
    };

} // namespace atmo::meta
