#pragma once

#include <optional>
#include <unordered_map>

#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/event/events/ui_event/dropdown_group_event/dropdown_group_event.hpp"

namespace atmo::core::components
{
    struct UiDropdownMenu {
        enum class OpenSide {
            BOTTOM,
            RIGHT
        };

        bool is_open = false;
        int group = 0;
        OpenSide open_side = OpenSide::BOTTOM;
        bool group_hovered = false;
        int depth = 0;
        uint32_t group_event_listener_id = 0;

        types::Vector2 aim_sample = { 0.0f, 0.0f };
        std::optional<float> aim_idle_time;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UiDropdownMenu> {
    static constexpr const char *name = "Dropdown";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::UiDropdownMenu::group>("group"));
};

namespace atmo::core::ecs::entities
{
    class UiDropdownMenu : public EntityRegistry::Registrable<UiDropdownMenu, UIButton>
    {
    public:
        using EntityRegistry::Registrable<UiDropdownMenu, UIButton>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UiDropdownMenu";
        }

        UILabel getLabel() const;
        UIRect getContent() const;

        void setText(const std::string &text);

        Clay_ElementDeclaration buildDecl() override;
        void draw(float delta_time) override;

    private:
        bool isHovered();
        void dispatchGroupEvent(event::events::DropdownGroupEvent::Action action);

        /**
         * @brief Checks if a movement starting at from points at the near edge of the content
         */
        bool isHeadingToContent(const types::Vector2 &from, const types::Vector2 &move) const;

        /**
         * @brief Tells if the pointer left this open dropdown and is still moving toward its content
         *
         * Can be called several times per frame, siblings call it as they may be drawn first.
         *
         * @param delta_time Time to add to the aim timer: the frame delta from this dropdown's own
         * draw, 0 from its siblings so that a frame is only counted once.
         */
        bool updateAim(float delta_time);

        /**
         * @brief Tells if the pointer is only crossing this dropdown on its way to the content of a sibling
         */
        bool isBlockedBySibling();

        static inline std::unordered_map<int, int> s_draw_depth;
        static inline std::unordered_map<flecs::entity_t, flecs::entity_t> s_open_child;

        static constexpr float AIM_MIN_MOVE = 4.0f;
        static constexpr float AIM_TIMEOUT = 0.15f;
    };
} // namespace atmo::core::ecs::entities
