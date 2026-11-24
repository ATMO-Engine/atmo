#include "scoped_voice.hpp"

#include <utility>

#include "server/audio/audio_server.hpp"

namespace atmo::server::audio
{
    ScopedVoice::~ScopedVoice()
    {
        reset();
    }

    ScopedVoice::ScopedVoice(const ScopedVoice &) noexcept {}

    ScopedVoice &ScopedVoice::operator=(const ScopedVoice &other)
    {
        if (this != &other)
            reset();
        return *this;
    }

    ScopedVoice::ScopedVoice(ScopedVoice &&other) noexcept : m_rid(std::exchange(other.m_rid, {})) {}

    ScopedVoice &ScopedVoice::operator=(ScopedVoice &&other)
    {
        if (this != &other) {
            reset();
            m_rid = std::exchange(other.m_rid, {});
        }
        return *this;
    }

    void ScopedVoice::create()
    {
        reset();
        m_rid = AudioServer::Instance().voiceCreate();
    }

    void ScopedVoice::reset()
    {
        if (m_rid.isValid())
            AudioServer::Instance().voiceFree(std::exchange(m_rid, {}));
    }

    bool ScopedVoice::isValid() const noexcept
    {
        return m_rid.isValid();
    }

    VoiceRid ScopedVoice::rid() const noexcept
    {
        return m_rid;
    }
} // namespace atmo::server::audio
