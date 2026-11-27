#pragma once

#include "server/audio/rid.hpp"

namespace atmo::server::audio
{
    /**
     * @brief Owns a voice and frees it when destroyed.
     * A voice cannot be shared, so a copy starts without one.
     */
    class ScopedVoice
    {
    public:
        ScopedVoice() noexcept = default;
        ~ScopedVoice();

        ScopedVoice(const ScopedVoice &other) noexcept;
        ScopedVoice &operator=(const ScopedVoice &other);
        ScopedVoice(ScopedVoice &&other) noexcept;
        ScopedVoice &operator=(ScopedVoice &&other);

        void create();
        void reset();

        [[nodiscard]] bool isValid() const noexcept;
        [[nodiscard]] VoiceRid rid() const noexcept;

    private:
        VoiceRid m_rid;
    };
} // namespace atmo::server::audio
