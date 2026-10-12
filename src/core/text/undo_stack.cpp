#include "undo_stack.hpp"
#include "core/text/utf8.hpp"

namespace atmo::core::text
{
    void UndoStack::push(Transaction transaction)
    {
        if (transaction.edits.empty())
            return;

        m_redo.clear();

        if (m_merge_allowed && tryMerge(transaction)) {
            return;
        }

        m_undo.push_back(std::move(transaction));
        if (m_undo.size() > limit)
            m_undo.erase(m_undo.begin());
        m_merge_allowed = true;
    }

    Transaction UndoStack::popUndo()
    {
        Transaction t = std::move(m_undo.back());
        m_undo.pop_back();
        m_redo.push_back(t);
        m_merge_allowed = false;
        return t;
    }

    Transaction UndoStack::popRedo()
    {
        Transaction t = std::move(m_redo.back());
        m_redo.pop_back();
        m_undo.push_back(t);
        m_merge_allowed = false;
        return t;
    }

    void UndoStack::clear() noexcept
    {
        m_undo.clear();
        m_redo.clear();
        m_merge_allowed = false;
    }

    bool UndoStack::tryMerge(const Transaction &transaction)
    {
        if (m_undo.empty() || transaction.kind == EditKind::Other)
            return false;

        auto &top = m_undo.back();
        if (top.kind != transaction.kind || top.edits.size() != 1 || transaction.edits.size() != 1)
            return false;
        if (transaction.time - top.time > merge_window)
            return false;

        auto &prev = top.edits.front();
        const auto &next = transaction.edits.front();

        switch (transaction.kind) {
            case EditKind::Typing:
                {
                    if (!next.removed.empty() || next.offset != prev.offset + prev.inserted.size())
                        return false;
                    // Start a new step when a word begins after whitespace ("hello |world").
                    auto last = Utf8::Decode(prev.inserted, Utf8::PrevBoundary(prev.inserted, prev.inserted.size()));
                    if (Utf8::IsWhitespace(last) && !Utf8::IsWhitespace(Utf8::Decode(next.inserted, 0)))
                        return false;
                    prev.inserted += next.inserted;
                    break;
                }
            case EditKind::Backspace:
                {
                    if (!next.inserted.empty() || next.offset + next.removed.size() != prev.offset)
                        return false;
                    prev.offset = next.offset;
                    prev.removed = next.removed + prev.removed;
                    break;
                }
            case EditKind::DeleteForward:
                {
                    if (!next.inserted.empty() || next.offset != prev.offset)
                        return false;
                    prev.removed += next.removed;
                    break;
                }
            default:
                return false;
        }

        top.selections_after = transaction.selections_after;
        top.time = transaction.time;
        return true;
    }
} // namespace atmo::core::text
