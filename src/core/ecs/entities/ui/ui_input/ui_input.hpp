#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "core/ecs/entities/ui/ui.hpp"
#include "core/text/text_editor.hpp"
#include "core/types.hpp"
#include "meta/field_descriptor.hpp"

namespace atmo::core::ecs::entities
{
    class UIInputState;
}

namespace atmo::core::components
{
    struct UIInput {
        enum class InputType {
            Text,
            Int,
            Float
        };

        /// Current text. Kept in sync with the editor on every edit.
        std::string text;
        /// Shown in `placeholder_color` while the text is empty. Translated through the locale manager.
        std::string placeholder;

        InputType input_type = InputType::Text;
        bool multiline = false;
        bool read_only = false;
        bool password = false;
        std::uint32_t max_length = 0;

        std::uint16_t font_size = 12;
        std::string font_path = "project://assets/fonts/Nunito/Nunito.ttf";

        types::Color text_color = types::Color::BLACK;
        types::Color placeholder_color = types::Color("#9CA3AF");
        types::Color caret_color = types::Color::BLACK;
        types::Color selection_color = types::Color("#93C5FD");
        types::Color background_color = types::Color::TRANSPARENT_COL;
        types::Color focus_color = types::Color("#DBEAFE");

        /// Runtime editing and rendering state, not serialized.
        std::shared_ptr<ecs::entities::UIInputState> m_state;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIInput> {
    static constexpr const char *name = "UIInput";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIInput::text>("text"), atmo::meta::field<&atmo::core::components::UIInput::placeholder>("placeholder"),
        atmo::meta::field<&atmo::core::components::UIInput::input_type>("input_type"),
        atmo::meta::field<&atmo::core::components::UIInput::multiline>("multiline"),
        atmo::meta::field<&atmo::core::components::UIInput::read_only>("read_only"), atmo::meta::field<&atmo::core::components::UIInput::password>("password"),
        atmo::meta::field<&atmo::core::components::UIInput::max_length>("max_length"),
        atmo::meta::field<&atmo::core::components::UIInput::font_size>("font_size"),
        atmo::meta::field<&atmo::core::components::UIInput::font_path>("font_path").withWidget("file_path"),
        atmo::meta::field<&atmo::core::components::UIInput::text_color>("text_color").withWidget("color_picker"),
        atmo::meta::field<&atmo::core::components::UIInput::placeholder_color>("placeholder_color").withWidget("color_picker"));
};

namespace atmo::core::ecs::entities
{
    /**
     * @brief Editable text field with caret, selection, clipboard, undo/redo and IME support.
     *
     * Editing is delegated to a `text::TextEditor`; this entity handles focus, input events and rendering.
     *
     * Signals:
     * - `TextChanged(std::string)`: after every edit.
     * - `Submitted(std::string)`: Enter pressed in a single-line input.
     * - `FocusChanged(bool)`.
     *
     * `commit()` runs on Enter and when focus is lost; subclasses convert and validate the text there.
     */
    class UIInput : public EntityRegistry::Registrable<UIInput, UI>
    {
    public:
        using EntityRegistry::Registrable<UIInput, UI>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIInput";
        }

        /// Replaces the text without emitting signals and resets the undo history.
        void setText(const std::string &text);
        [[nodiscard]] std::string getText() const;

        void setPlaceholder(const std::string &placeholder);
        [[nodiscard]] std::string getPlaceholder() const;

        void focus();
        void blur();
        [[nodiscard]] bool isFocused() const;

        text::TextEditor &editor();

        /// Called on Enter and on blur. Subclasses validate and publish the value here.
        virtual void commit() {}
        /// Restores the default/empty value.
        virtual void clear();

        Clay_ElementDeclaration buildDecl() override;
        void draw() override;

    protected:
        /// Applies the component's options (multiline, filters...) to the editor. Subclasses may extend the filter.
        virtual void configureEditor(text::TextEditor &editor);

        UIInputState &state();

    private:
        void handlePointer(UIInputState &state);
        void handleTextEvents(UIInputState &state);
        void syncFromEditor(UIInputState &state);
        [[nodiscard]] std::size_t hitTest(UIInputState &state) const;
    };
} // namespace atmo::core::ecs::entities
