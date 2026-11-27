#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace atmo::server::audio
{
    enum class RidKind : std::uint8_t {
        Sample,
        Voice
    };

    template <RidKind K> class RidAllocator;

    template <RidKind K> class Rid
    {
    public:
        static constexpr RidKind kind = K;
        static constexpr std::uint32_t slot_bits = 24;
        static constexpr std::uint32_t slot_mask = (1u << slot_bits) - 1u;

        constexpr Rid() noexcept = default;

        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return m_value != 0;
        }

        [[nodiscard]] constexpr std::uint32_t raw() const noexcept
        {
            return m_value;
        }

        [[nodiscard]] constexpr std::uint32_t slot() const noexcept
        {
            return m_value & slot_mask;
        }

        [[nodiscard]] constexpr std::uint8_t generation() const noexcept
        {
            return static_cast<std::uint8_t>(m_value >> slot_bits);
        }

        constexpr bool operator==(const Rid &) const noexcept = default;

    private:
        friend class RidAllocator<K>;

        constexpr Rid(std::uint32_t slot, std::uint8_t generation) noexcept : m_value((static_cast<std::uint32_t>(generation) << slot_bits) | slot) {}

        std::uint32_t m_value = 0;
    };

    using SampleRid = Rid<RidKind::Sample>;
    using VoiceRid = Rid<RidKind::Voice>;
} // namespace atmo::server::audio

template <atmo::server::audio::RidKind K> struct std::hash<atmo::server::audio::Rid<K>> {
    std::size_t operator()(const atmo::server::audio::Rid<K> &rid) const noexcept
    {
        return std::hash<std::uint32_t>{}(rid.raw());
    }
};
