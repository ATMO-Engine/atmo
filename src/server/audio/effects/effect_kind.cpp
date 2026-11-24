#include "effect_kind.hpp"

#include "server/audio/effects/delay.hpp"
#include "server/audio/effects/ducker.hpp"
#include "server/audio/effects/filters.hpp"
#include "server/audio/effects/reverb.hpp"

namespace atmo::server::audio
{
    static_assert(HighPass::parameters.size() <= max_effect_parameters);
    static_assert(LowPass::parameters.size() <= max_effect_parameters);
    static_assert(Eq3Band::parameters.size() <= max_effect_parameters);
    static_assert(Delay::parameters.size() <= max_effect_parameters);
    static_assert(Reverb::parameters.size() <= max_effect_parameters);
    static_assert(Ducker::parameters.size() <= max_effect_parameters);

    std::span<const EffectParameter> EffectParameters(EffectKind kind) noexcept
    {
        switch (kind) {
            case EffectKind::HighPass:
                return HighPass::parameters;
            case EffectKind::LowPass:
                return LowPass::parameters;
            case EffectKind::Eq3Band:
                return Eq3Band::parameters;
            case EffectKind::Delay:
                return Delay::parameters;
            case EffectKind::Reverb:
                return Reverb::parameters;
        }
        return {};
    }

    std::unique_ptr<AudioEffect> CreateEffect(EffectKind kind)
    {
        switch (kind) {
            case EffectKind::HighPass:
                return std::make_unique<HighPass>();
            case EffectKind::LowPass:
                return std::make_unique<LowPass>();
            case EffectKind::Eq3Band:
                return std::make_unique<Eq3Band>();
            case EffectKind::Delay:
                return std::make_unique<Delay>();
            case EffectKind::Reverb:
                return std::make_unique<Reverb>();
        }
        return nullptr;
    }
} // namespace atmo::server::audio
