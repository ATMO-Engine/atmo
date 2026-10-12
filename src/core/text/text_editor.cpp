#include "text_editor.hpp"
#include <algorithm>
#include "core/text/utf8.hpp"

namespace atmo::core::text
{
    TextEditor::TextEditor(Options options) : m_options(std::move(options)) {}

    void TextEditor::setText(std::string_view text, bool reset_undo)
    {
        if (reset_undo) {
            m_buffer.setText(text);
            m_undo.clear();
        } else {
            applyEdits({ { 0, 0, m_buffer.size(), std::string(text) } }, EditKind::Other);
            m_undo.breakMerge();
        }
        m_selections = { Selection::Caret(m_buffer.size()) };
        m_preedit = {};
        ++m_text_revision;
        touchState();
    }

    void TextEditor::setSelection(Selection selection)
    {
        selection.anchor = m_buffer.clamp(selection.anchor);
        selection.head = m_buffer.clamp(selection.head);
        m_selections = { selection };
        m_undo.breakMerge();
        touchState();
    }

    std::string TextEditor::selectedText() const
    {
        std::string out;
        for (const auto &sel : m_selections) {
            if (sel.empty())
                continue;
            if (!out.empty())
                out += '\n';
            out += m_buffer.slice(sel.min(), sel.max());
        }
        return out;
    }

    bool TextEditor::execute(EditAction action, TextLayout *layout)
    {
        switch (action.command) {
            case EditCommand::MoveLeft:
            case EditCommand::MoveRight:
            case EditCommand::MoveWordLeft:
            case EditCommand::MoveWordRight:
            case EditCommand::MoveLineStart:
            case EditCommand::MoveLineEnd:
            case EditCommand::MoveDocStart:
            case EditCommand::MoveDocEnd:
            case EditCommand::MoveUp:
            case EditCommand::MoveDown:
            case EditCommand::MovePageUp:
            case EditCommand::MovePageDown:
                return move(action.command, action.extend, layout);

            case EditCommand::Backspace:
                return deleteRanges(
                    [this](const Selection &s) { return std::pair{ Utf8::PrevBoundary(m_buffer.text(), s.head), s.head }; }, EditKind::Backspace);
            case EditCommand::Delete:
                return deleteRanges(
                    [this](const Selection &s) { return std::pair{ s.head, Utf8::NextBoundary(m_buffer.text(), s.head) }; }, EditKind::DeleteForward);
            case EditCommand::DeleteWordBack:
                return deleteRanges([this](const Selection &s) { return std::pair{ Utf8::WordStart(m_buffer.text(), s.head), s.head }; }, EditKind::Other);
            case EditCommand::DeleteWordForward:
                return deleteRanges([this](const Selection &s) { return std::pair{ s.head, Utf8::WordEnd(m_buffer.text(), s.head) }; }, EditKind::Other);
            case EditCommand::DeleteToLineStart:
                return deleteRanges(
                    [this](const Selection &s) {
                        auto start = m_buffer.lineRange(m_buffer.lineOf(s.head)).first;
                        // At the start of a line, join with the previous one.
                        return std::pair{ start == s.head ? Utf8::PrevBoundary(m_buffer.text(), s.head) : start, s.head };
                    },
                    EditKind::Other);
            case EditCommand::DeleteToLineEnd:
                return deleteRanges(
                    [this](const Selection &s) {
                        auto end = m_buffer.lineRange(m_buffer.lineOf(s.head)).second;
                        return std::pair{ s.head, end == s.head ? Utf8::NextBoundary(m_buffer.text(), s.head) : end };
                    },
                    EditKind::Other);

            case EditCommand::InsertNewline:
                if (!m_options.multiline)
                    return false;
                return insertText("\n");
            case EditCommand::InsertTab:
                if (!m_options.tab_inserts)
                    return false;
                return insertText("\t");

            case EditCommand::SelectAll:
                {
                    auto all = Selection{ 0, m_buffer.size(), std::nullopt };
                    if (m_selections.size() == 1 && m_selections.front() == all)
                        return false;
                    setSelection(all);
                    return true;
                }
            case EditCommand::Undo:
                return undoRedo(false);
            case EditCommand::Redo:
                return undoRedo(true);

            case EditCommand::Copy:
                {
                    auto selected = selectedText();
                    if (!m_clipboard || m_options.password || selected.empty())
                        return false;
                    m_clipboard->setText(selected);
                    return true;
                }
            case EditCommand::Cut:
                {
                    if (m_options.read_only || m_options.password || !m_clipboard)
                        return false;
                    auto selected = selectedText();
                    if (selected.empty())
                        return false;
                    m_clipboard->setText(selected);
                    std::vector<PendingEdit> edits;
                    for (std::size_t i = 0; i < m_selections.size(); ++i) edits.push_back({ i, m_selections[i].min(), m_selections[i].max(), {} });
                    m_undo.breakMerge();
                    return applyEdits(std::move(edits), EditKind::Other);
                }
            case EditCommand::Paste:
                {
                    if (!m_clipboard)
                        return false;
                    m_undo.breakMerge();
                    bool changed = insertText(m_clipboard->getText());
                    m_undo.breakMerge();
                    return changed;
                }
        }
        return false;
    }

    bool TextEditor::insertText(std::string_view text)
    {
        if (m_options.read_only)
            return false;

        auto filtered = filterInput(text);
        if (filtered.empty())
            return false;

        std::vector<PendingEdit> edits;
        std::size_t available = 0;
        if (m_options.max_length > 0) {
            std::size_t current = Utf8::CodepointCount(m_buffer.text());
            std::size_t selected = 0;
            for (const auto &sel : m_selections) selected += Utf8::CodepointCount(m_buffer.slice(sel.min(), sel.max()));
            available = m_options.max_length > current - selected ? m_options.max_length - (current - selected) : 0;
        }

        for (std::size_t i = 0; i < m_selections.size(); ++i) {
            std::string piece = filtered;
            if (m_options.max_length > 0) {
                std::size_t end = 0, count = 0;
                while (end < piece.size() && count < available) {
                    end = Utf8::NextBoundary(piece, end);
                    ++count;
                }
                piece.resize(end);
                available -= count;
            }
            edits.push_back({ i, m_selections[i].min(), m_selections[i].max(), std::move(piece) });
        }

        bool replaces = std::ranges::any_of(m_selections, [](const Selection &s) { return !s.empty(); });
        bool single_char = Utf8::NextBoundary(filtered, 0) == filtered.size();
        if (replaces || !single_char)
            m_undo.breakMerge();

        bool changed = applyEdits(std::move(edits), replaces || !single_char ? EditKind::Other : EditKind::Typing);
        if (replaces || !single_char)
            m_undo.breakMerge();
        return changed;
    }

    void TextEditor::clickAt(std::size_t offset, int clicks, bool extend)
    {
        offset = m_buffer.clamp(offset);
        m_undo.breakMerge();

        if (clicks >= 3) {
            m_drag_granularity = Granularity::Line;
            m_drag_origin = lineRangeAt(offset);
            m_selections = { Selection{ m_drag_origin.first, m_drag_origin.second, std::nullopt } };
        } else if (clicks == 2) {
            m_drag_granularity = Granularity::Word;
            m_drag_origin = Utf8::WordAt(m_buffer.text(), offset);
            m_selections = { Selection{ m_drag_origin.first, m_drag_origin.second, std::nullopt } };
        } else {
            m_drag_granularity = Granularity::Char;
            if (extend) {
                auto anchor = primary().anchor;
                m_selections = { Selection{ anchor, offset, std::nullopt } };
                m_drag_origin = { anchor, anchor };
            } else {
                m_selections = { Selection::Caret(offset) };
                m_drag_origin = { offset, offset };
            }
        }
        touchState();
    }

    void TextEditor::dragTo(std::size_t offset)
    {
        offset = m_buffer.clamp(offset);
        std::pair<std::size_t, std::size_t> range{ offset, offset };

        switch (m_drag_granularity) {
            case Granularity::Char:
                break;
            case Granularity::Word:
                range = Utf8::WordAt(m_buffer.text(), offset);
                break;
            case Granularity::Line:
                range = lineRangeAt(offset);
                break;
        }

        Selection sel;
        if (range.first < m_drag_origin.first) {
            sel = { m_drag_origin.second, range.first, std::nullopt };
        } else {
            sel = { m_drag_origin.first, std::max(range.second, m_drag_origin.second), std::nullopt };
        }

        if (m_selections.size() == 1 && m_selections.front() == sel)
            return;
        m_selections = { sel };
        touchState();
    }

    void TextEditor::setPreedit(std::string text, std::size_t cursor)
    {
        if (m_preedit.text == text && m_preedit.cursor == cursor)
            return;
        m_preedit = { std::move(text), cursor };
        touchState();
    }

    void TextEditor::clearPreedit()
    {
        setPreedit({}, 0);
    }

    std::string TextEditor::displayText() const
    {
        if (!m_options.password)
            return m_buffer.text();
        std::string out;
        auto count = Utf8::CodepointCount(m_buffer.text());
        out.reserve(count * PASSWORD_MASK.size());
        for (std::size_t i = 0; i < count; ++i) out += PASSWORD_MASK;
        return out;
    }

    std::size_t TextEditor::toDisplayOffset(std::size_t offset) const
    {
        if (!m_options.password)
            return offset;
        return Utf8::CodepointCount(m_buffer.slice(0, offset)) * PASSWORD_MASK.size();
    }

    std::size_t TextEditor::fromDisplayOffset(std::size_t offset) const
    {
        if (!m_options.password)
            return m_buffer.clamp(offset);
        auto index = offset / PASSWORD_MASK.size();
        std::size_t out = 0;
        for (std::size_t i = 0; i < index && out < m_buffer.size(); ++i) out = Utf8::NextBoundary(m_buffer.text(), out);
        return out;
    }

    bool TextEditor::applyEdits(std::vector<PendingEdit> edits, EditKind kind)
    {
        std::ranges::sort(edits, {}, &PendingEdit::start);

        Transaction transaction;
        transaction.kind = kind;
        transaction.time = m_clock();
        transaction.selections_before = m_selections;

        std::vector<Selection> after = m_selections;
        std::ptrdiff_t delta = 0;

        for (auto &edit : edits) {
            if (edit.start == edit.end && edit.text.empty())
                continue;
            auto start = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(edit.start) + delta);
            auto end = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(edit.end) + delta);

            Edit record{ start, std::string(m_buffer.slice(start, end)), edit.text };
            m_buffer.erase(start, end - start);
            m_buffer.insert(start, edit.text);

            after[edit.selection_index] = Selection::Caret(start + edit.text.size());
            delta += static_cast<std::ptrdiff_t>(edit.text.size()) - static_cast<std::ptrdiff_t>(edit.end - edit.start);
            transaction.edits.push_back(std::move(record));
        }

        if (transaction.edits.empty())
            return false;

        m_selections = std::move(after);
        normalizeSelections();
        transaction.selections_after = m_selections;
        m_undo.push(std::move(transaction));

        ++m_text_revision;
        touchState();
        return true;
    }

    bool TextEditor::deleteRanges(const std::function<std::pair<std::size_t, std::size_t>(const Selection &)> &range_for_caret, EditKind kind)
    {
        if (m_options.read_only)
            return false;

        std::vector<PendingEdit> edits;
        bool any_selection = false;
        for (std::size_t i = 0; i < m_selections.size(); ++i) {
            const auto &sel = m_selections[i];
            if (!sel.empty()) {
                any_selection = true;
                edits.push_back({ i, sel.min(), sel.max(), {} });
            } else {
                auto [start, end] = range_for_caret(sel);
                edits.push_back({ i, start, end, {} });
            }
        }

        if (any_selection)
            m_undo.breakMerge();
        bool changed = applyEdits(std::move(edits), any_selection ? EditKind::Other : kind);
        if (any_selection || kind == EditKind::Other)
            m_undo.breakMerge();
        return changed;
    }

    bool TextEditor::move(EditCommand command, bool extend, TextLayout *layout)
    {
        auto before = m_selections;
        bool vertical =
            command == EditCommand::MoveUp || command == EditCommand::MoveDown || command == EditCommand::MovePageUp || command == EditCommand::MovePageDown;

        for (auto &sel : m_selections) {
            std::optional<float> preferred_x = vertical ? sel.preferred_x : std::nullopt;

            // Collapsing a selection with Left/Right goes to the respective edge instead of moving.
            if (!extend && !sel.empty() && (command == EditCommand::MoveLeft || command == EditCommand::MoveRight)) {
                auto edge = command == EditCommand::MoveLeft ? sel.min() : sel.max();
                sel = Selection::Caret(edge);
                continue;
            }

            auto target = moveOffset(command, sel, layout, preferred_x);
            if (extend)
                sel = { sel.anchor, target, preferred_x };
            else
                sel = { target, target, preferred_x };
        }

        normalizeSelections();
        m_undo.breakMerge();

        if (before == m_selections)
            return false;
        touchState();
        return true;
    }

    std::size_t TextEditor::moveOffset(EditCommand command, const Selection &sel, TextLayout *layout, std::optional<float> &preferred_x) const
    {
        const auto &str = m_buffer.text();
        switch (command) {
            case EditCommand::MoveLeft:
                return Utf8::PrevBoundary(str, sel.head);
            case EditCommand::MoveRight:
                return Utf8::NextBoundary(str, sel.head);
            case EditCommand::MoveWordLeft:
                return Utf8::WordStart(str, sel.head);
            case EditCommand::MoveWordRight:
                return Utf8::WordEnd(str, sel.head);
            case EditCommand::MoveLineStart:
                return m_buffer.lineRange(m_buffer.lineOf(sel.head)).first;
            case EditCommand::MoveLineEnd:
                return m_buffer.lineRange(m_buffer.lineOf(sel.head)).second;
            case EditCommand::MoveDocStart:
                return 0;
            case EditCommand::MoveDocEnd:
                return m_buffer.size();
            case EditCommand::MoveUp:
                return verticalMove(sel, -(layout ? layout->lineHeight() : 1.0f), layout, preferred_x);
            case EditCommand::MoveDown:
                return verticalMove(sel, layout ? layout->lineHeight() : 1.0f, layout, preferred_x);
            case EditCommand::MovePageUp:
                return verticalMove(sel, -(layout ? layout->pageHeight() : 10.0f), layout, preferred_x);
            case EditCommand::MovePageDown:
                return verticalMove(sel, layout ? layout->pageHeight() : 10.0f, layout, preferred_x);
            default:
                return sel.head;
        }
    }

    std::size_t TextEditor::verticalMove(const Selection &sel, float dy, TextLayout *layout, std::optional<float> &preferred_x) const
    {
        if (layout) {
            layout->setText(displayText());
            auto caret = layout->caretRect(toDisplayOffset(sel.head));
            if (!preferred_x)
                preferred_x = caret.x;
            float target_y = caret.y + dy + caret.height * 0.5f;
            if (target_y < 0.0f)
                return 0;
            if (dy > 0 && layout->caretRect(toDisplayOffset(m_buffer.size())).y + caret.height <= target_y)
                return m_buffer.size();
            return fromDisplayOffset(layout->offsetAt(*preferred_x, target_y));
        }

        // Without a layout: logical lines and codepoint columns.
        auto pos = m_buffer.offsetToLineCol(sel.head);
        auto [line_start, _] = m_buffer.lineRange(pos.line);
        if (!preferred_x)
            preferred_x = static_cast<float>(Utf8::CodepointCount(m_buffer.slice(line_start, sel.head)));

        auto lines = static_cast<long long>(dy);
        auto target_line = static_cast<long long>(pos.line) + lines;
        if (target_line < 0)
            return 0;
        if (target_line >= static_cast<long long>(m_buffer.lineCount()))
            return m_buffer.size();

        auto [start, end] = m_buffer.lineRange(static_cast<std::size_t>(target_line));
        auto offset = start;
        for (auto col = static_cast<long long>(*preferred_x); col > 0 && offset < end; --col) offset = Utf8::NextBoundary(m_buffer.text(), offset);
        return offset;
    }

    bool TextEditor::undoRedo(bool redo)
    {
        if (m_options.read_only || (redo ? !m_undo.canRedo() : !m_undo.canUndo()))
            return false;

        auto transaction = redo ? m_undo.popRedo() : m_undo.popUndo();
        if (redo) {
            for (const auto &edit : transaction.edits) {
                m_buffer.erase(edit.offset, edit.removed.size());
                m_buffer.insert(edit.offset, edit.inserted);
            }
            m_selections = transaction.selections_after;
        } else {
            for (auto it = transaction.edits.rbegin(); it != transaction.edits.rend(); ++it) {
                m_buffer.erase(it->offset, it->inserted.size());
                m_buffer.insert(it->offset, it->removed);
            }
            m_selections = transaction.selections_before;
        }

        ++m_text_revision;
        touchState();
        return true;
    }

    std::string TextEditor::filterInput(std::string_view text) const
    {
        std::string out;
        out.reserve(text.size());
        for (std::size_t i = 0; i < text.size(); i = Utf8::NextBoundary(text, i)) {
            char32_t cp = Utf8::Decode(text, i);
            if (cp == U'\r')
                continue;
            if (cp == U'\n' && !m_options.multiline)
                cp = U' ';
            if (cp == U'\t' && !m_options.multiline && !m_options.tab_inserts)
                cp = U' ';
            if (cp < 0x20 && cp != U'\n' && cp != U'\t')
                continue;
            if (cp == 0x7F || cp == Utf8::INVALID)
                continue;
            if (m_options.char_filter && !m_options.char_filter(cp))
                continue;
            Utf8::Append(out, cp);
        }
        return out;
    }

    void TextEditor::normalizeSelections()
    {
        if (m_selections.size() < 2)
            return;

        std::ranges::sort(m_selections, {}, &Selection::min);
        std::vector<Selection> merged;
        for (const auto &sel : m_selections) {
            if (!merged.empty() && sel.min() <= merged.back().max()) {
                auto &last = merged.back();
                auto lo = std::min(last.min(), sel.min());
                auto hi = std::max(last.max(), sel.max());
                last = last.anchor <= last.head ? Selection{ lo, hi, std::nullopt } : Selection{ hi, lo, std::nullopt };
            } else {
                merged.push_back(sel);
            }
        }
        m_selections = std::move(merged);
    }

    std::pair<std::size_t, std::size_t> TextEditor::lineRangeAt(std::size_t offset) const
    {
        auto [start, end] = m_buffer.lineRange(m_buffer.lineOf(offset));
        // Include the newline so that triple-click + delete removes the whole line.
        if (end < m_buffer.size())
            ++end;
        return { start, end };
    }
} // namespace atmo::core::text
