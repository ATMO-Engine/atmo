#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "core/text/selection.hpp"
#include "core/text/text_buffer.hpp"
#include "core/text/text_layout.hpp"
#include "core/text/undo_stack.hpp"

namespace atmo::core::text
{
    class IClipboard
    {
    public:
        virtual ~IClipboard() = default;
        virtual void setText(std::string_view text) = 0;
        [[nodiscard]] virtual std::string getText() const = 0;
    };

    enum class EditCommand {
        MoveLeft,
        MoveRight,
        MoveWordLeft,
        MoveWordRight,
        MoveLineStart,
        MoveLineEnd,
        MoveDocStart,
        MoveDocEnd,
        MoveUp,
        MoveDown,
        MovePageUp,
        MovePageDown,

        Backspace,
        Delete,
        DeleteWordBack,
        DeleteWordForward,
        DeleteToLineStart,
        DeleteToLineEnd,
        InsertNewline,
        InsertTab,

        SelectAll,
        Undo,
        Redo,
        Copy,
        Cut,
        Paste
    };

    /// An editing command. `extend` turns moves into selection-extending moves (Shift held).
    struct EditAction {
        EditCommand command = EditCommand::MoveLeft;
        bool extend = false;

        bool operator==(const EditAction &) const = default;
    };

    struct TextEditorOptions {
        bool multiline = false;
        bool read_only = false;
        bool password = false;
        bool tab_inserts = false;
        std::uint32_t max_length = 0; ///< Maximum number of codepoints, 0 = unlimited.
        std::function<bool(char32_t)> char_filter;
    };

    /// IME composition text, shown at the caret but not part of the buffer until committed.
    struct Preedit {
        std::string text;
        std::size_t cursor = 0; ///< Byte offset of the IME cursor inside `text`.
    };

    /**
     * @brief Reusable text-editing controller: buffer, selections, undo history and editing commands.
     *
     * It has no dependency on SDL, rendering or ECS so it can back both single-line UI inputs and a code editor.
     * Geometry-dependent commands (vertical moves, mouse hit-testing) take a `TextLayout`; without one they fall back
     * to logical lines and codepoint columns.
     *
     * Selections are kept in a vector to allow multi-cursor editing later; UI inputs only ever use one.
     */
    class TextEditor
    {
    public:
        using Clock = std::function<std::chrono::steady_clock::time_point()>;

        using Options = TextEditorOptions;

        explicit TextEditor(Options options = {});

        [[nodiscard]] const TextBuffer &buffer() const noexcept
        {
            return m_buffer;
        }
        [[nodiscard]] const std::string &text() const noexcept
        {
            return m_buffer.text();
        }
        /// Replaces the whole text and puts the caret at the end. Bypasses filters and read-only.
        void setText(std::string_view text, bool reset_undo = true);

        [[nodiscard]] const std::vector<Selection> &selections() const noexcept
        {
            return m_selections;
        }
        [[nodiscard]] const Selection &primary() const noexcept
        {
            return m_selections.front();
        }
        void setSelection(Selection selection);
        [[nodiscard]] std::string selectedText() const;

        /// Executes a command. Returns false when the command did nothing (e.g. Backspace at offset 0).
        bool execute(EditAction action, TextLayout *layout = nullptr);
        /// Types or pastes text at every selection, applying filters, `max_length` and newline handling.
        bool insertText(std::string_view text);

        /// Mouse press at `offset`: 1 click places the caret, 2 selects a word, 3 selects a line.
        void clickAt(std::size_t offset, int clicks, bool extend = false);
        /// Mouse drag to `offset`, extending by the granularity of the initial click.
        void dragTo(std::size_t offset);

        void setPreedit(std::string text, std::size_t cursor);
        void clearPreedit();
        [[nodiscard]] const Preedit &preedit() const noexcept
        {
            return m_preedit;
        }

        /// Whether a placeholder should be displayed instead of the text.
        [[nodiscard]] bool showsPlaceholder() const noexcept
        {
            return m_buffer.empty() && m_preedit.text.empty();
        }

        /// Text as it should be displayed (masked when `password` is set).
        [[nodiscard]] std::string displayText() const;
        /// Maps a buffer offset to the corresponding offset in `displayText()`.
        [[nodiscard]] std::size_t toDisplayOffset(std::size_t offset) const;
        /// Maps an offset in `displayText()` back to a buffer offset.
        [[nodiscard]] std::size_t fromDisplayOffset(std::size_t offset) const;

        /// Incremented on every text change.
        [[nodiscard]] std::uint64_t textRevision() const noexcept
        {
            return m_text_revision;
        }
        /// Incremented on every text, selection or preedit change.
        [[nodiscard]] std::uint64_t stateRevision() const noexcept
        {
            return m_state_revision;
        }

        Options &options() noexcept
        {
            return m_options;
        }
        [[nodiscard]] const Options &options() const noexcept
        {
            return m_options;
        }

        UndoStack &undoStack() noexcept
        {
            return m_undo;
        }

        void setClipboard(IClipboard *clipboard) noexcept
        {
            m_clipboard = clipboard;
        }
        void setClock(Clock clock)
        {
            m_clock = std::move(clock);
        }

        static constexpr std::string_view PASSWORD_MASK = "•";

    private:
        struct PendingEdit {
            std::size_t selection_index;
            std::size_t start;
            std::size_t end;
            std::string text;
        };

        enum class Granularity {
            Char,
            Word,
            Line
        };

        bool applyEdits(std::vector<PendingEdit> edits, EditKind kind);
        bool deleteRanges(const std::function<std::pair<std::size_t, std::size_t>(const Selection &)> &range_for_caret, EditKind kind);
        bool move(EditCommand command, bool extend, TextLayout *layout);
        std::size_t moveOffset(EditCommand command, const Selection &sel, TextLayout *layout, std::optional<float> &preferred_x) const;
        std::size_t verticalMove(const Selection &sel, float dy, TextLayout *layout, std::optional<float> &preferred_x) const;
        bool undoRedo(bool redo);
        std::string filterInput(std::string_view text) const;
        void normalizeSelections();
        void touchState() noexcept
        {
            ++m_state_revision;
        }
        std::pair<std::size_t, std::size_t> lineRangeAt(std::size_t offset) const;

        TextBuffer m_buffer;
        std::vector<Selection> m_selections{ Selection::Caret(0) };
        UndoStack m_undo;
        Options m_options;
        Preedit m_preedit;
        IClipboard *m_clipboard = nullptr;
        Clock m_clock = [] { return std::chrono::steady_clock::now(); };

        Granularity m_drag_granularity = Granularity::Char;
        std::pair<std::size_t, std::size_t> m_drag_origin{ 0, 0 };

        std::uint64_t m_text_revision = 0;
        std::uint64_t m_state_revision = 0;
    };
} // namespace atmo::core::text
