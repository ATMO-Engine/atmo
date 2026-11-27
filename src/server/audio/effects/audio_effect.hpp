#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace atmo::server::audio
{
    struct EffectParameter {
        std::string_view name;
        float min;
        float max;
        float initial;
    };

    /**
     * @brief Processes a bus in place, on the audio thread: process() must not allocate, lock or log.
     */
    class AudioEffect
    {
    public:
        AudioEffect() = default;
        virtual ~AudioEffect() = default;

        AudioEffect(const AudioEffect &) = delete;
        AudioEffect &operator=(const AudioEffect &) = delete;

        virtual void process(float *stereo, std::size_t frames) = 0;

        /**
         * @brief Apply new settings without resetting the effect.
         *
         * @param values One value per parameter of the effect, already in range
         */
        virtual void setParameters(std::span<const float> values) = 0;
    };
} // namespace atmo::server::audio
