#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>
#include "core/text/selection.hpp"

namespace atmo::core::text
{
    /// A single replacement at `offset`: `removed` was replaced by `inserted`.
    struct Edit {
        std::size_t offset = 0;
        std::string removed;
        std::string inserted;
    };

    enum class EditKind {
        Other,
        Typing,
        Backspace,
        DeleteForward
    };

    /// A group of edits undone/redone as one step, with the selections to restore on either side.
    struct Transaction {
        std::vector<Edit> edits;
        std::vector<Selection> selections_before;
        std::vector<Selection> selections_after;
        EditKind kind = EditKind::Other;
        std::chrono::steady_clock::time_point time;
    };

    /**
     * @brief Linear undo/redo history with coalescing of consecutive typing and deletions.
     *
     * Coalescing stops when the caret moved in between (`breakMerge()`), when the idle gap exceeds `merge_window`,
     * when the edit kind changes, or when a word starts after whitespace while typing.
     */
    class UndoStack
    {
    public:
        static constexpr std::chrono::milliseconds DEFAULT_MERGE_WINDOW{ 1000 };

        void push(Transaction transaction);

        /// Prevents the next pushed transaction from being merged into the current top.
        void breakMerge() noexcept
        {
            m_merge_allowed = false;
        }

        [[nodiscard]] bool canUndo() const noexcept
        {
            return !m_undo.empty();
        }
        [[nodiscard]] bool canRedo() const noexcept
        {
            return !m_redo.empty();
        }

        /// Pops the transaction to revert. The caller applies the edits in reverse.
        Transaction popUndo();
        /// Pops the transaction to re-apply.
        Transaction popRedo();

        void clear() noexcept;

        std::chrono::milliseconds merge_window = DEFAULT_MERGE_WINDOW;
        std::size_t limit = 1000;

    private:
        bool tryMerge(const Transaction &transaction);

        std::vector<Transaction> m_undo;
        std::vector<Transaction> m_redo;
        bool m_merge_allowed = false;
    };
} // namespace atmo::core::text
