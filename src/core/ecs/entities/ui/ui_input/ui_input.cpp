#include "ui_input.hpp"
#include <algorithm>
#include <cmath>
#include <variant>
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_timer.h"
#include "clay.h"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_input/ttf_text_layout.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/input/clipboard.hpp"
#include "core/input/focus_manager.hpp"
#include "core/input/input_manager.hpp"
#include "core/text/keymap.hpp"
#include "core/text/utf8.hpp"
#include "locale/locale_manager.hpp"
#include "meta/auto_register.hpp"
#include "server/rendering/rendering_server.hpp"

namespace atmo::core::ecs::entities
{
    /// Runtime state of a UIInput: the editor model plus everything needed to draw it.
    class UIInputState : public ClayCustomRenderable
    {
    public:
        static constexpr Uint64 BLINK_PERIOD_MS = 530;
        static constexpr float MIN_WIDTH = 40.0f;

        text::TextEditor editor;
        /// Geometry of the displayed text (masked for passwords), used for hit-testing and caret placement.
        TtfTextLayout layout;
        /// Placeholder, or text with the IME composition spliced in.
        TtfTextLayout overlay;

        std::string font_path;
        float font_size = 12.0f;
        float dpi_x = 1.0f;
        float dpi_y = 1.0f;

        types::Color text_color = types::Color::BLACK;
        types::Color placeholder_color = types::Color::BLACK;
        types::Color caret_color = types::Color::BLACK;
        types::Color selection_color = types::Color::BLACK;
        std::string placeholder;

        bool focused = false;
        bool dragging = false;
        bool cancel_on_blur = false;
        std::string focus_text;
        std::string synced_text;
        std::uint64_t synced_revision = 0;
        std::uint64_t seen_state_revision = 0;
        Uint64 activity_ms = 0;
        components::UIInput::InputType configured_type = components::UIInput::InputType::Text;
        bool configured = false;

        Clay_ElementId text_element_id{};

        /// Pixels scrolled to keep the caret visible.
        float scroll_x = 0.0f;
        float scroll_y = 0.0f;
        /// Vertical offset of the text inside its element, in pixels (single-line centering).
        float text_offset_y = 0.0f;
        /// Caret rect in window (logical) coordinates, used to place the IME candidate window.
        SDL_Rect ime_rect{};
        SDL_Rect sent_ime_rect{ -1, -1, -1, -1 };

        bool ensureFont()
        {
            bool ok = layout.setFont(font_path, font_size, 96.0f * dpi_x, 96.0f * dpi_y);
            overlay.setFont(font_path, font_size, 96.0f * dpi_x, 96.0f * dpi_y);
            return ok;
        }

        [[nodiscard]] bool multiline() const noexcept
        {
            return editor.options().multiline;
        }

        void render(server::rendering::CanvasCommandWriter &writer, const server::rendering::Rect2 &bounds, float raster_scale) override
        {
            const float scale = raster_scale > 0.0f ? raster_scale : 1.0f;
            const SDL_FRect rect{ bounds.position.x * scale, bounds.position.y * scale, bounds.size.x * scale, bounds.size.y * scale };
            const auto fill = [&writer, scale](const SDL_FRect &pixels, const types::Color &color) {
                writer.addRect({ { pixels.x / scale, pixels.y / scale }, { pixels.w / scale, pixels.h / scale } }, color);
            };
            dpi_x = scale;
            dpi_y = scale;
            if (!ensureFont())
                return;

            int wrap = multiline() ? std::max(1, static_cast<int>(rect.w)) : 0;
            layout.setWrapWidth(wrap);
            overlay.setWrapWidth(wrap);
            layout.setText(editor.displayText());
            layout.setPageHeight(rect.h);

            const auto &primary = editor.primary();
            const auto &preedit = editor.preedit();
            const std::size_t caret_display = editor.toDisplayOffset(primary.head);

            TtfTextLayout *drawn = &layout;
            types::Color color = text_color;
            text::CaretRect caret = layout.caretRect(caret_display);
            std::vector<SDL_FRect> underline;

            if (editor.showsPlaceholder()) {
                overlay.setText(placeholder);
                drawn = &overlay;
                color = placeholder_color;
                caret = { 0.0f, 0.0f, layout.lineHeight() };
            } else if (!preedit.text.empty()) {
                std::string composed = editor.displayText();
                composed.insert(caret_display, preedit.text);
                overlay.setText(composed);
                drawn = &overlay;
                caret = overlay.caretRect(caret_display + preedit.cursor);
                underline = overlay.rangeRects(caret_display, caret_display + preedit.text.size());
            }

            const float caret_width = std::max(1.0f, std::round(dpi_x));
            auto content = drawn == &overlay && editor.showsPlaceholder() ? layout.size() : drawn->size();

            if (focused) {
                if (caret.x - scroll_x > rect.w - caret_width)
                    scroll_x = caret.x - rect.w + caret_width;
                if (caret.x < scroll_x)
                    scroll_x = caret.x;
                if (multiline()) {
                    if (caret.y + caret.height - scroll_y > rect.h)
                        scroll_y = caret.y + caret.height - rect.h;
                    if (caret.y < scroll_y)
                        scroll_y = caret.y;
                }
            }
            scroll_x = std::clamp(scroll_x, 0.0f, std::max(0.0f, static_cast<float>(content.x) + caret_width - rect.w));
            scroll_y = multiline() ? std::clamp(scroll_y, 0.0f, std::max(0.0f, static_cast<float>(content.y) - rect.h)) : 0.0f;
            text_offset_y = multiline() ? 0.0f : std::round((rect.h - layout.lineHeight()) * 0.5f);

            const float ox = rect.x - scroll_x;
            const float oy = rect.y + text_offset_y - scroll_y;

            writer.pushClip(bounds);

            if (focused && preedit.text.empty() && !editor.showsPlaceholder()) {
                for (const auto &sel : editor.selections()) {
                    if (sel.empty())
                        continue;
                    for (auto r : layout.rangeRects(editor.toDisplayOffset(sel.min()), editor.toDisplayOffset(sel.max()))) {
                        r.x += ox;
                        r.y += oy;
                        fill(r, selection_color);
                    }
                }
            }

            drawn->draw(writer, ox, oy, color, scale);

            for (const auto &r : underline) fill({ ox + r.x, oy + r.y + r.h - caret_width, r.w, caret_width }, text_color);

            const bool blink_on = ((SDL_GetTicks() - activity_ms) / BLINK_PERIOD_MS) % 2 == 0;
            if (focused && !editor.options().read_only && blink_on)
                fill({ std::round(ox + caret.x), oy + caret.y, caret_width, caret.height }, caret_color);

            writer.popClip();

            ime_rect = { static_cast<int>((ox + caret.x) / dpi_x),
                         static_cast<int>((oy + caret.y) / dpi_y),
                         static_cast<int>(caret_width / dpi_x),
                         static_cast<int>(caret.height / dpi_y) };
        }
    };

    namespace
    {
        types::Color Modulated(const types::Color &color, const types::Color &modulate)
        {
            return color * modulate;
        }
    } // namespace

    void UIInput::RegisterSystems(flecs::world *world)
    {
        world->observer<components::UIInput>("UIInput_remove").event(flecs::OnRemove).each([](flecs::entity e, components::UIInput &) {
            FocusManager::Release(e.id());
        });
    }

    void UIInput::initialize()
    {
        UI::initialize();

        setComponent<components::UIInput>({});

        createSignal<std::string>("TextChanged");
        createSignal<std::string>("Submitted");
        createSignal<bool>("FocusChanged");

        auto &layout = getComponentMutable<components::Layout>();
        layout.padding = { 6, 6, 4, 4 };
        layout.child_alignment.vertical = components::Layout::ChildAlignment::Center;

        state();
    }

    UIInputState &UIInput::state()
    {
        auto &comp = getComponentMutable<components::UIInput>();
        if (!comp.m_state) {
            comp.m_state = std::make_shared<UIInputState>();
            auto &st = *comp.m_state;
            st.editor.setClipboard(&SdlClipboard::Instance());
            st.text_element_id = Clay_GetElementIdWithIndex(CLAY_STRING("AtmoUIInputText"), static_cast<uint32_t>(getID()));
            st.editor.setText(comp.text);
            st.synced_text = comp.text;
            st.synced_revision = st.editor.textRevision();
        }
        return *comp.m_state;
    }

    void UIInput::configureEditor(text::TextEditor &editor)
    {
        const auto &comp = getComponent<components::UIInput>();
        auto &options = editor.options();
        options.multiline = comp.multiline;
        options.read_only = comp.read_only;
        options.password = comp.password;
        options.max_length = comp.max_length;
        options.tab_inserts = comp.multiline;

        switch (comp.input_type) {
            case components::UIInput::InputType::Int:
                options.char_filter = [](char32_t c) { return (c >= U'0' && c <= U'9') || c == U'-' || c == U'+'; };
                break;
            case components::UIInput::InputType::Float:
                options.char_filter = [](char32_t c) { return (c >= U'0' && c <= U'9') || c == U'-' || c == U'+' || c == U'.' || c == U'e' || c == U'E'; };
                break;
            case components::UIInput::InputType::Text:
                options.char_filter = nullptr;
                break;
        }
    }

    void UIInput::setText(const std::string &text)
    {
        auto &st = state();
        auto &comp = getComponentMutable<components::UIInput>();
        st.editor.setText(text);
        comp.text = text;
        st.synced_text = text;
        st.synced_revision = st.editor.textRevision();
        if (st.focused)
            st.focus_text = text;
    }

    std::string UIInput::getText() const
    {
        return getComponent<components::UIInput>().text;
    }

    void UIInput::setPlaceholder(const std::string &placeholder)
    {
        getComponentMutable<components::UIInput>().placeholder = placeholder;
    }

    std::string UIInput::getPlaceholder() const
    {
        return getComponent<components::UIInput>().placeholder;
    }

    text::TextEditor &UIInput::editor()
    {
        return state().editor;
    }

    bool UIInput::isFocused() const
    {
        return FocusManager::IsFocused(getID());
    }

    void UIInput::clear()
    {
        setText({});
    }

    void UIInput::focus()
    {
        auto &st = state();
        if (st.focused && FocusManager::IsFocused(getID()))
            return;

        SDL_Window *sdl_window = nullptr;
        if (auto window = getWindow())
            sdl_window = server::rendering::RenderingServer::Instance().windowGetSdlWindow(window->getComponent<components::Window>().rid);

        flecs::entity handle = p_handle;
        auto with_input = [handle](auto &&fn) {
            if (!handle.is_alive())
                return false;
            auto wrapped = EntityRegistry::Wrap(handle);
            if (auto *input = dynamic_cast<UIInput *>(wrapped.get())) {
                fn(*input);
                return true;
            }
            return false;
        };

        FocusManager::Focus(
            {
                .id = getID(),
                .window = sdl_window,
                .on_blur =
                    [with_input] {
                        with_input([](UIInput &input) {
                            auto &s = input.state();
                            s.focused = false;
                            s.dragging = false;
                            s.editor.clearPreedit();

                            if (s.cancel_on_blur) {
                                s.cancel_on_blur = false;
                                s.editor.setText(s.focus_text, false);
                            }
                            input.syncFromEditor(s);
                            input.commit();
                            input.getSignal<bool>("FocusChanged").emit(false);
                        });
                    },
                .on_action =
                    [with_input](text::EditAction action) {
                        bool handled = false;
                        with_input([&](UIInput &input) {
                            auto &s = input.state();
                            handled = s.editor.execute(action, &s.layout);
                        });
                        return handled;
                    },
            });

        st.focused = true;
        st.focus_text = st.editor.text();
        st.activity_ms = SDL_GetTicks();
        st.sent_ime_rect = { -1, -1, -1, -1 };
        getSignal<bool>("FocusChanged").emit(true);
    }

    void UIInput::blur()
    {
        if (FocusManager::IsFocused(getID()))
            FocusManager::Blur();
    }

    std::size_t UIInput::hitTest(UIInputState &st) const
    {
        Clay_ElementData element = Clay_GetElementData(st.text_element_id);
        if (!element.found)
            return st.editor.primary().head;

        auto mouse = InputManager::GetMousePosition();
        float x = (mouse.x - element.boundingBox.x) * st.dpi_x + st.scroll_x;
        float y = st.multiline() ? (mouse.y - element.boundingBox.y) * st.dpi_y + st.scroll_y : st.layout.lineHeight() * 0.5f;

        st.layout.setText(st.editor.displayText());
        return st.editor.fromDisplayOffset(st.layout.offsetAt(x, y));
    }

    void UIInput::handlePointer(UIInputState &st)
    {
        const bool hovered = Clay_Hovered();

        if (InputManager::IsJustPressed("ui_click")) {
            if (hovered) {
                focus();
                st.ensureFont();
                const bool extend = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
                st.editor.clickAt(hitTest(st), InputManager::GetLastClickCount(), extend);
                st.dragging = true;
            } else if (st.focused) {
                blur();
            }
        }

        if (st.dragging) {
            if (InputManager::IsPressed("ui_click"))
                st.editor.dragTo(hitTest(st));
            else
                st.dragging = false;
        }
    }

    void UIInput::handleTextEvents(UIInputState &st)
    {
        st.ensureFont();
        st.layout.setText(st.editor.displayText());

        for (auto &event : InputManager::ConsumeTextEvents()) {
            if (!st.focused)
                break;

            if (auto *key = std::get_if<TextKeyDown>(&event)) {
                if (!st.editor.preedit().text.empty())
                    continue;

                if (key->key == SDLK_ESCAPE) {
                    st.cancel_on_blur = true;
                    blur();
                    break;
                }

                auto action = text::Keymap::Lookup(key->key, key->mod);
                if (!action)
                    continue;

                if (action->command == text::EditCommand::InsertNewline && !st.multiline()) {
                    std::string submitted = st.editor.text();
                    blur();
                    getSignal<std::string>("Submitted").emit(submitted);
                    break;
                }
                if (action->command == text::EditCommand::InsertTab && !st.editor.options().tab_inserts)
                    continue;

                st.editor.execute(*action, &st.layout);
                st.layout.setText(st.editor.displayText());
            } else if (auto *commit = std::get_if<TextCommit>(&event)) {
                st.editor.clearPreedit();
                st.editor.insertText(commit->text);
                st.layout.setText(st.editor.displayText());
            } else if (auto *composition = std::get_if<TextComposition>(&event)) {
                std::size_t cursor = 0;
                for (int i = 0; i < composition->start && cursor < composition->text.size(); ++i) cursor = text::Utf8::NextBoundary(composition->text, cursor);
                st.editor.setPreedit(composition->text, cursor);
            }
        }
    }

    void UIInput::syncFromEditor(UIInputState &st)
    {
        auto &comp = getComponentMutable<components::UIInput>();

        if (comp.text != st.synced_text) {
            st.editor.setText(comp.text);
            st.synced_text = comp.text;
            st.synced_revision = st.editor.textRevision();
        }

        if (st.editor.textRevision() != st.synced_revision) {
            comp.text = st.editor.text();
            st.synced_text = comp.text;
            st.synced_revision = st.editor.textRevision();
            getSignal<std::string>("TextChanged").emit(comp.text);
        }
    }

    Clay_ElementDeclaration UIInput::buildDecl()
    {
        Clay_ElementDeclaration d = UI::buildDecl();
        const auto &ui = getComponent<components::UI>();
        const auto &comp = getComponent<components::UIInput>();
        const bool focused = comp.m_state && comp.m_state->focused;

        d.backgroundColor = ((focused ? comp.focus_color : comp.background_color) * ui.modulate).toFloat<Clay_Color>(255);
        d.cornerRadius = { 4, 4, 4, 4 };
        return d;
    }

    void UIInput::draw()
    {
        auto &st = state();
        const auto &ui = getComponent<components::UI>();

        if (auto window = getWindow()) {
            auto scale = window->getDPIScale();
            st.dpi_x = scale.x;
            st.dpi_y = scale.y;
        }

        {
            const auto &comp = getComponent<components::UIInput>();
            if (!st.configured || st.configured_type != comp.input_type) {
                configureEditor(st.editor);
                st.configured_type = comp.input_type;
                st.configured = true;
            }
            st.editor.options().multiline = comp.multiline;
            st.editor.options().read_only = comp.read_only;
            st.editor.options().password = comp.password;
            st.editor.options().max_length = comp.max_length;
            st.font_path = comp.font_path;
            st.font_size = comp.font_size;
            st.text_color = Modulated(comp.text_color, ui.modulate);
            st.placeholder_color = Modulated(comp.placeholder_color, ui.modulate);
            st.caret_color = Modulated(comp.caret_color, ui.modulate);
            st.selection_color = Modulated(comp.selection_color, ui.modulate);
            st.placeholder = locale::LocaleManager::GetTranslation(comp.placeholder);
        }

        if (st.focused && !FocusManager::IsFocused(getID()))
            st.focused = false;

        handlePointer(st);
        if (st.focused)
            handleTextEvents(st);
        syncFromEditor(st);

        if (st.editor.stateRevision() != st.seen_state_revision) {
            st.seen_state_revision = st.editor.stateRevision();
            st.activity_ms = SDL_GetTicks();
        }

        if (st.focused && (st.ime_rect.x != st.sent_ime_rect.x || st.ime_rect.y != st.sent_ime_rect.y || st.ime_rect.h != st.sent_ime_rect.h)) {
            if (auto window = getWindow()) {
                InputManager::SetTextInputArea(
                    server::rendering::RenderingServer::Instance().windowGetSdlWindow(window->getComponent<components::Window>().rid), st.ime_rect, 0);
                st.sent_ime_rect = st.ime_rect;
            }
        }

        st.ensureFont();
        const float line_height = st.layout.lineHeight() / std::max(st.dpi_y, 0.01f);
        const auto content = st.editor.showsPlaceholder() ? st.overlay.size() : st.layout.size();
        const float content_width = static_cast<float>(content.x) / std::max(st.dpi_x, 0.01f) + 1.0f;

        Clay_ElementDeclaration text_decl{};
        text_decl.id = st.text_element_id;
        text_decl.layout.sizing.width = CLAY_SIZING_GROW(std::max(UIInputState::MIN_WIDTH, content_width));
        text_decl.layout.sizing.height = st.multiline() ? CLAY_SIZING_GROW(line_height) : CLAY_SIZING_FIXED(line_height);
        text_decl.custom.customData = static_cast<ClayCustomRenderable *>(&st);

        Clay__OpenElement();
        Clay__ConfigureOpenElement(text_decl);
        Clay__CloseElement();
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIInput);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIInput)
