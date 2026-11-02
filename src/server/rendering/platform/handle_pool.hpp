#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace atmo::server::rendering
{
    template <typename Handle, typename T> class HandlePool
    {
    public:
        static constexpr std::uint32_t slot_bits = 24;
        static constexpr std::uint32_t slot_mask = (1u << slot_bits) - 1u;

        [[nodiscard]] Handle insert(T value)
        {
            std::uint32_t slot;
            if (!m_free_slots.empty()) {
                slot = m_free_slots.back();
                m_free_slots.pop_back();
            } else {
                if (m_slots.size() > slot_mask)
                    throw std::length_error("GPU handle slots exhausted");
                slot = static_cast<std::uint32_t>(m_slots.size());
                m_slots.emplace_back();
            }

            Slot &entry = m_slots[slot];
            entry.value.emplace(std::move(value));
            ++m_size;
            return Handle{ (static_cast<std::uint32_t>(entry.generation) << slot_bits) | slot };
        }

        [[nodiscard]] T *get(Handle handle) noexcept
        {
            Slot *entry = find(handle);
            return entry ? &*entry->value : nullptr;
        }

        [[nodiscard]] const T *get(Handle handle) const noexcept
        {
            const Slot *entry = const_cast<HandlePool *>(this)->find(handle);
            return entry ? &*entry->value : nullptr;
        }

        [[nodiscard]] T &at(Handle handle)
        {
            T *value = get(handle);
            if (!value)
                throw std::invalid_argument("invalid or stale GPU handle");
            return *value;
        }

        [[nodiscard]] const T &at(Handle handle) const
        {
            const T *value = get(handle);
            if (!value)
                throw std::invalid_argument("invalid or stale GPU handle");
            return *value;
        }

        std::optional<T> remove(Handle handle)
        {
            Slot *entry = find(handle);
            if (!entry)
                return std::nullopt;

            std::optional<T> value = std::move(entry->value);
            entry->value.reset();
            entry->generation = entry->generation == 255 ? 1 : static_cast<std::uint8_t>(entry->generation + 1);
            m_free_slots.push_back(handle.value & slot_mask);
            --m_size;
            return value;
        }

        template <typename F> void forEach(F &&function)
        {
            for (std::size_t slot = 0; slot < m_slots.size(); ++slot) {
                Slot &entry = m_slots[slot];
                if (entry.value)
                    function(Handle{ (static_cast<std::uint32_t>(entry.generation) << slot_bits) | static_cast<std::uint32_t>(slot) }, *entry.value);
            }
        }

        void clear() noexcept
        {
            m_slots.clear();
            m_free_slots.clear();
            m_size = 0;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return m_size;
        }

    private:
        struct Slot {
            std::optional<T> value;
            std::uint8_t generation = 1;
        };

        Slot *find(Handle handle) noexcept
        {
            if (!handle.isValid())
                return nullptr;

            const std::uint32_t slot = handle.value & slot_mask;
            const auto generation = static_cast<std::uint8_t>(handle.value >> slot_bits);
            if (slot >= m_slots.size())
                return nullptr;

            Slot &entry = m_slots[slot];
            return entry.value && entry.generation == generation ? &entry : nullptr;
        }

        std::vector<Slot> m_slots;
        std::vector<std::uint32_t> m_free_slots;
        std::size_t m_size = 0;
    };
} // namespace atmo::server::rendering
