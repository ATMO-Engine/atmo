#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "rid.hpp"

namespace atmo::server::rendering
{
    template <RidKind K> class RidAllocator
    {
    public:
        static constexpr std::uint32_t max_slots = Rid<K>::slot_mask + 1u;

        [[nodiscard]] Rid<K> allocate()
        {
            std::uint32_t slot;
            if (!m_free_slots.empty()) {
                slot = m_free_slots.back();
                m_free_slots.pop_back();
            } else {
                if (m_slots.size() >= max_slots)
                    throw std::length_error("RID slots exhausted");
                slot = static_cast<std::uint32_t>(m_slots.size());
                m_slots.push_back({});
            }

            m_slots[slot].alive = true;
            ++m_alive_count;
            return Rid<K>(slot, m_slots[slot].generation);
        }

        bool release(Rid<K> rid)
        {
            if (!isAlive(rid))
                return false;

            Slot &entry = m_slots[rid.slot()];
            entry.alive = false;
            entry.generation = entry.generation == 255 ? 1 : static_cast<std::uint8_t>(entry.generation + 1);
            m_free_slots.push_back(rid.slot());
            --m_alive_count;
            return true;
        }

        [[nodiscard]] bool isAlive(Rid<K> rid) const noexcept
        {
            if (!rid.isValid() || rid.slot() >= m_slots.size())
                return false;

            const Slot &entry = m_slots[rid.slot()];
            return entry.alive && entry.generation == rid.generation();
        }

        [[nodiscard]] std::size_t aliveCount() const noexcept
        {
            return m_alive_count;
        }

        [[nodiscard]] std::size_t capacity() const noexcept
        {
            return m_slots.size();
        }

    private:
        struct Slot {
            std::uint8_t generation = 1;
            bool alive = false;
        };

        std::vector<Slot> m_slots;
        std::vector<std::uint32_t> m_free_slots;
        std::size_t m_alive_count = 0;
    };
} // namespace atmo::server::rendering
