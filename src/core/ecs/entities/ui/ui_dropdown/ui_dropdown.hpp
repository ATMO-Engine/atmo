#pragma once

#include <unordered_map>

#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/event/events/ui_event/dropdown_group_event/dropdown_group_event.hpp"

namespace atmo::core::components
{
    struct UIDropdown {
        bool is_open = false;
        int group = 0;
        bool group_hovered = false;
        int depth = 0;
        uint32_t group_event_listener_id = 0;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIDropdown> {
    static constexpr const char *name = "Dropdown";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIDropdown::group>("group"));
};

namespace atmo::core::ecs::entities
{
    class UIDropdown : public EntityRegistry::Registrable<UIDropdown, UIButton>
    {
    public:
        using EntityRegistry::Registrable<UIDropdown, UIButton>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIDropdown";
        }

        UILabel getLabel() const;
        UIRect getContent() const;

        void setText(const std::string &text);

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;
    private:
        bool isHovered();
        void dispatchGroupEvent(event::events::DropdownGroupEvent::Action action);

        // Number of open dropdowns currently being drawn above this point, per group
        static inline std::unordered_map<int, int> s_draw_depth;
    };
} // namespace atmo::core::ecs::entities
