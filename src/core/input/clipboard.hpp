#pragma once

#include <string>
#include <string_view>
#include "core/text/text_editor.hpp"

namespace atmo::core
{
    /// System clipboard backed by SDL.
    class SdlClipboard : public text::IClipboard
    {
    public:
        void setText(std::string_view text) override;
        [[nodiscard]] std::string getText() const override;

        static SdlClipboard &Instance()
        {
            static SdlClipboard instance;
            return instance;
        }
    };
} // namespace atmo::core
