#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include "core/text/keymap.hpp"
#include "core/text/text_buffer.hpp"
#include "core/text/text_editor.hpp"
#include "core/text/utf8.hpp"

using namespace atmo::core::text;

namespace
{
    /// Each codepoint is 1 unit wide, each line 1 unit tall.
    class MonospaceLayout : public TextLayout
    {
    public:
        void setText(std::string_view text) override
        {
            m_buffer.setText(text);
        }

        CaretRect caretRect(std::size_t offset) const override
        {
            auto pos = m_buffer.offsetToLineCol(offset);
            auto [start, _] = m_buffer.lineRange(pos.line);
            return { static_cast<float>(Utf8::CodepointCount(m_buffer.slice(start, offset))), static_cast<float>(pos.line), 1.0f };
        }

        std::size_t offsetAt(float x, float y) const override
        {
            if (y < 0)
                return 0;
            auto line = static_cast<std::size_t>(y);
            if (line >= m_buffer.lineCount())
                return m_buffer.size();
            auto [start, end] = m_buffer.lineRange(line);
            auto offset = start;
            for (int col = static_cast<int>(x + 0.5f); col > 0 && offset < end; --col) offset = Utf8::NextBoundary(m_buffer.text(), offset);
            return offset;
        }

        float lineHeight() const override
        {
            return 1.0f;
        }

        float pageHeight() const override
        {
            return 2.0f;
        }

    private:
        TextBuffer m_buffer;
    };

    class FakeClipboard : public IClipboard
    {
    public:
        void setText(std::string_view text) override
        {
            content = text;
        }
        std::string getText() const override
        {
            return content;
        }
        std::string content;
    };

    struct FakeClock {
        std::chrono::steady_clock::time_point now;
        TextEditor::Clock fn()
        {
            return [this] { return now; };
        }
    };

    EditAction Cmd(EditCommand c, bool extend = false)
    {
        return { c, extend };
    }

    void TypeEach(TextEditor &editor, std::string_view s)
    {
        for (std::size_t i = 0; i < s.size(); i = Utf8::NextBoundary(s, i)) editor.insertText(s.substr(i, Utf8::NextBoundary(s, i) - i));
    }
} // namespace

TEST_CASE("Utf8 boundaries", "[text]")
{
    std::string s = "a\xC3\xA9\xF0\x9F\x99\x82z"; // a é 🙂 z
    REQUIRE(Utf8::NextBoundary(s, 0) == 1);
    REQUIRE(Utf8::NextBoundary(s, 1) == 3);
    REQUIRE(Utf8::NextBoundary(s, 3) == 7);
    REQUIRE(Utf8::NextBoundary(s, 7) == 8);
    REQUIRE(Utf8::NextBoundary(s, 8) == 8);
    REQUIRE(Utf8::PrevBoundary(s, 8) == 7);
    REQUIRE(Utf8::PrevBoundary(s, 7) == 3);
    REQUIRE(Utf8::PrevBoundary(s, 3) == 1);
    REQUIRE(Utf8::PrevBoundary(s, 0) == 0);
    REQUIRE(Utf8::SnapToBoundary(s, 5) == 3);
    REQUIRE(Utf8::CodepointCount(s) == 4);
    REQUIRE(Utf8::Decode(s, 3) == 0x1F642);

    std::string out;
    Utf8::Append(out, 0x1F642);
    Utf8::Append(out, U'é');
    REQUIRE(out == "\xF0\x9F\x99\x82\xC3\xA9");
}

TEST_CASE("Utf8 word boundaries", "[text]")
{
    std::string s = "foo.bar  baz";
    REQUIRE(Utf8::WordEnd(s, 0) == 3);
    REQUIRE(Utf8::WordEnd(s, 3) == 4);
    REQUIRE(Utf8::WordEnd(s, 7) == 12);
    REQUIRE(Utf8::WordStart(s, 12) == 9);
    REQUIRE(Utf8::WordStart(s, 9) == 4);
    REQUIRE(Utf8::WordStart(s, 4) == 3);
    REQUIRE(Utf8::WordAt(s, 5) == std::pair<std::size_t, std::size_t>{ 4, 7 });
    REQUIRE(Utf8::WordAt(s, 12) == std::pair<std::size_t, std::size_t>{ 9, 12 });
}

TEST_CASE("TextBuffer line index", "[text]")
{
    TextBuffer b("ab\ncd\n\nef");
    REQUIRE(b.lineCount() == 4);
    REQUIRE(b.lineRange(1) == std::pair<std::size_t, std::size_t>{ 3, 5 });
    REQUIRE(b.lineRange(2) == std::pair<std::size_t, std::size_t>{ 6, 6 });
    REQUIRE(b.offsetToLineCol(4) == LineCol{ 1, 1 });
    REQUIRE(b.lineColToOffset({ 3, 1 }) == 8);
    REQUIRE(b.lineColToOffset({ 0, 99 }) == 2);

    b.insert(1, "X\nY");
    REQUIRE(b.text() == "aX\nYb\ncd\n\nef");
    REQUIRE(b.lineCount() == 5);
    b.erase(2, 1);
    REQUIRE(b.text() == "aXYb\ncd\n\nef");
    REQUIRE(b.lineCount() == 4);

    for (std::size_t i = 0; i <= b.size(); ++i) REQUIRE(b.lineColToOffset(b.offsetToLineCol(i)) == i);
}

TEST_CASE("TextEditor typing and caret movement", "[text]")
{
    TextEditor e;
    TypeEach(e, "hello");
    REQUIRE(e.text() == "hello");
    REQUIRE(e.primary().head == 5);

    e.execute(Cmd(EditCommand::MoveLeft));
    e.execute(Cmd(EditCommand::MoveLeft));
    e.insertText("X");
    REQUIRE(e.text() == "helXlo");

    e.execute(Cmd(EditCommand::MoveLineStart));
    REQUIRE(e.primary().head == 0);
    REQUIRE_FALSE(e.execute(Cmd(EditCommand::MoveLeft)));
    e.execute(Cmd(EditCommand::MoveLineEnd));
    REQUIRE(e.primary().head == 6);
}

TEST_CASE("TextEditor never splits multi-byte characters", "[text]")
{
    TextEditor e;
    e.setText("a\xC3\xA9\xF0\x9F\x99\x82");
    e.execute(Cmd(EditCommand::Backspace));
    REQUIRE(e.text() == "a\xC3\xA9");
    e.execute(Cmd(EditCommand::MoveLeft));
    REQUIRE(e.primary().head == 1);
    e.execute(Cmd(EditCommand::Delete));
    REQUIRE(e.text() == "a");
}

TEST_CASE("TextEditor selection", "[text]")
{
    TextEditor e;
    e.setText("hello world");

    e.execute(Cmd(EditCommand::MoveWordLeft, true));
    REQUIRE(e.selectedText() == "world");

    e.insertText("there");
    REQUIRE(e.text() == "hello there");

    e.execute(Cmd(EditCommand::SelectAll));
    REQUIRE(e.selectedText() == "hello there");
    e.execute(Cmd(EditCommand::MoveLeft));
    REQUIRE(e.primary().empty());
    REQUIRE(e.primary().head == 0);

    e.execute(Cmd(EditCommand::MoveRight, true));
    e.execute(Cmd(EditCommand::MoveRight, true));
    REQUIRE(e.selectedText() == "he");
    e.execute(Cmd(EditCommand::Backspace));
    REQUIRE(e.text() == "llo there");
}

TEST_CASE("TextEditor word deletion", "[text]")
{
    TextEditor e;
    e.setText("foo bar baz");
    e.execute(Cmd(EditCommand::DeleteWordBack));
    REQUIRE(e.text() == "foo bar ");
    e.execute(Cmd(EditCommand::MoveDocStart));
    e.execute(Cmd(EditCommand::DeleteWordForward));
    REQUIRE(e.text() == " bar ");
}

TEST_CASE("TextEditor mouse clicks", "[text]")
{
    TextEditor e;
    e.setText("one two\nthree");

    e.clickAt(5, 1);
    REQUIRE(e.primary().empty());
    REQUIRE(e.primary().head == 5);

    e.clickAt(5, 2);
    REQUIRE(e.selectedText() == "two");

    e.clickAt(1, 3);
    REQUIRE(e.selectedText() == "one two\n");

    e.clickAt(1, 1);
    e.dragTo(6);
    REQUIRE(e.selectedText() == "ne tw");

    e.clickAt(5, 2);
    e.dragTo(1);
    REQUIRE(e.selectedText() == "one two");
    REQUIRE(e.primary().head == 0);

    e.clickAt(0, 1);
    e.clickAt(3, 1, true);
    REQUIRE(e.selectedText() == "one");
}

TEST_CASE("TextEditor vertical movement keeps preferred column", "[text]")
{
    MonospaceLayout layout;
    TextEditor e({ .multiline = true });
    e.setText("abcdef\nab\nabcdef");
    e.setSelection(Selection::Caret(5));

    e.execute(Cmd(EditCommand::MoveDown), &layout);
    REQUIRE(e.primary().head == 9);
    e.execute(Cmd(EditCommand::MoveDown), &layout);
    REQUIRE(e.primary().head == 15);
    e.execute(Cmd(EditCommand::MoveDown), &layout);
    REQUIRE(e.primary().head == e.text().size());
    e.execute(Cmd(EditCommand::MovePageUp), &layout);
    REQUIRE(e.primary().head == 5);

    // Without a layout, logical lines and codepoint columns are used.
    e.setSelection(Selection::Caret(5));
    e.execute(Cmd(EditCommand::MoveDown));
    e.execute(Cmd(EditCommand::MoveDown));
    REQUIRE(e.primary().head == 15);
    e.execute(Cmd(EditCommand::MoveUp, true));
    REQUIRE(e.selectedText() == "\nabcde");
}

TEST_CASE("TextEditor single-line input options", "[text]")
{
    TextEditor e;
    e.insertText("a\r\nb\tc");
    REQUIRE(e.text() == "a b c");
    REQUIRE_FALSE(e.execute(Cmd(EditCommand::InsertNewline)));

    TextEditor limited({ .max_length = 4 });
    limited.insertText("abcdef");
    REQUIRE(limited.text() == "abcd");
    REQUIRE_FALSE(limited.insertText("x"));
    limited.execute(Cmd(EditCommand::MoveLeft, true));
    REQUIRE(limited.insertText("\xC3\xA9xyz"));
    REQUIRE(limited.text() == "abc\xC3\xA9");

    TextEditor digits({ .char_filter = [](char32_t c) { return c >= U'0' && c <= U'9'; } });
    digits.insertText("1a2b3");
    REQUIRE(digits.text() == "123");

    TextEditor ro({ .read_only = true });
    ro.setText("fixed");
    REQUIRE_FALSE(ro.insertText("x"));
    REQUIRE_FALSE(ro.execute(Cmd(EditCommand::Backspace)));
    REQUIRE(ro.execute(Cmd(EditCommand::MoveLeft)));
    REQUIRE(ro.text() == "fixed");
}

TEST_CASE("TextEditor clipboard", "[text]")
{
    FakeClipboard clipboard;
    TextEditor e;
    e.setClipboard(&clipboard);
    e.setText("copy me");

    e.execute(Cmd(EditCommand::MoveWordLeft, true));
    REQUIRE(e.execute(Cmd(EditCommand::Copy)));
    REQUIRE(clipboard.content == "me");

    REQUIRE(e.execute(Cmd(EditCommand::Cut)));
    REQUIRE(e.text() == "copy ");

    e.execute(Cmd(EditCommand::MoveDocStart));
    REQUIRE(e.execute(Cmd(EditCommand::Paste)));
    REQUIRE(e.text() == "mecopy ");

    REQUIRE(e.execute(Cmd(EditCommand::Undo)));
    REQUIRE(e.text() == "copy ");

    TextEditor pw({ .password = true });
    pw.setClipboard(&clipboard);
    pw.setText("s\xC3\xA9");
    pw.execute(Cmd(EditCommand::SelectAll));
    REQUIRE_FALSE(pw.execute(Cmd(EditCommand::Copy)));
    REQUIRE(pw.displayText() == "••");
    REQUIRE(pw.toDisplayOffset(3) == 6);
    REQUIRE(pw.fromDisplayOffset(3) == 1);
}

TEST_CASE("TextEditor undo coalescing", "[text]")
{
    FakeClock clock;
    TextEditor e;
    e.setClock(clock.fn());

    TypeEach(e, "hello");
    e.execute(Cmd(EditCommand::Undo));
    REQUIRE(e.text().empty());
    e.execute(Cmd(EditCommand::Redo));
    REQUIRE(e.text() == "hello");
    REQUIRE(e.primary().head == 5);

    // A word after whitespace starts a new step.
    TypeEach(e, " world");
    e.execute(Cmd(EditCommand::Undo));
    REQUIRE(e.text() == "hello ");

    // A caret move breaks merging.
    e.execute(Cmd(EditCommand::MoveLeft));
    e.execute(Cmd(EditCommand::MoveRight));
    TypeEach(e, "ab");
    TypeEach(e, "c");
    e.execute(Cmd(EditCommand::Undo));
    REQUIRE(e.text() == "hello ");

    // An idle gap breaks merging.
    TypeEach(e, "x");
    clock.now += std::chrono::seconds(2);
    TypeEach(e, "y");
    e.execute(Cmd(EditCommand::Undo));
    REQUIRE(e.text() == "hello x");

    // Consecutive backspaces are one step and restore the caret.
    e.execute(Cmd(EditCommand::Backspace));
    e.execute(Cmd(EditCommand::Backspace));
    REQUIRE(e.text() == "hello");
    e.execute(Cmd(EditCommand::Undo));
    REQUIRE(e.text() == "hello x");
    REQUIRE(e.primary().head == 7);

    // A new edit clears redo.
    e.execute(Cmd(EditCommand::Undo));
    REQUIRE(e.undoStack().canRedo());
    e.insertText("!");
    REQUIRE_FALSE(e.undoStack().canRedo());
}

TEST_CASE("TextEditor undo restores replaced selection", "[text]")
{
    TextEditor e;
    e.setText("abc");
    e.execute(Cmd(EditCommand::SelectAll));
    e.insertText("z");
    REQUIRE(e.text() == "z");
    e.execute(Cmd(EditCommand::Undo));
    REQUIRE(e.text() == "abc");
    REQUIRE(e.selectedText() == "abc");
}

TEST_CASE("TextEditor placeholder visibility and preedit", "[text]")
{
    TextEditor e;
    REQUIRE(e.showsPlaceholder());
    e.setPreedit("\xE3\x81\x82", 3);
    REQUIRE_FALSE(e.showsPlaceholder());
    e.clearPreedit();
    REQUIRE(e.showsPlaceholder());
    e.insertText("a");
    REQUIRE_FALSE(e.showsPlaceholder());
}

TEST_CASE("TextEditor revisions", "[text]")
{
    TextEditor e;
    auto text_rev = e.textRevision();
    auto state_rev = e.stateRevision();
    e.insertText("a");
    REQUIRE(e.textRevision() > text_rev);
    text_rev = e.textRevision();
    e.execute(Cmd(EditCommand::MoveLeft));
    REQUIRE(e.textRevision() == text_rev);
    REQUIRE(e.stateRevision() > state_rev);
}

TEST_CASE("Keymap platform conventions", "[text]")
{
    using P = Keymap::Platform;
    REQUIRE(Keymap::Lookup(SDLK_LEFT, SDL_KMOD_ALT, P::MacOS) == EditAction{ EditCommand::MoveWordLeft });
    REQUIRE(Keymap::Lookup(SDLK_LEFT, SDL_KMOD_GUI | SDL_KMOD_SHIFT, P::MacOS) == EditAction{ EditCommand::MoveLineStart, true });
    REQUIRE(Keymap::Lookup(SDLK_LEFT, SDL_KMOD_CTRL, P::Other) == EditAction{ EditCommand::MoveWordLeft });
    REQUIRE(Keymap::Lookup(SDLK_Z, SDL_KMOD_GUI, P::MacOS) == EditAction{ EditCommand::Undo });
    REQUIRE(Keymap::Lookup(SDLK_Z, SDL_KMOD_GUI | SDL_KMOD_SHIFT, P::MacOS) == EditAction{ EditCommand::Redo });
    REQUIRE(Keymap::Lookup(SDLK_Y, SDL_KMOD_CTRL, P::Other) == EditAction{ EditCommand::Redo });
    REQUIRE_FALSE(Keymap::Lookup(SDLK_Z, SDL_KMOD_CTRL, P::MacOS).has_value());
    REQUIRE(Keymap::Lookup(SDLK_BACKSPACE, SDL_KMOD_NONE, P::Other) == EditAction{ EditCommand::Backspace });
    REQUIRE_FALSE(Keymap::Lookup(SDLK_S, SDL_KMOD_CTRL, P::Other).has_value());
}
