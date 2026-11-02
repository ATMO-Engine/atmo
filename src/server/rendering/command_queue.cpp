#include "command_queue.hpp"

#include <algorithm>
#include <exception>

#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
    CommandQueue::Arena::Arena(std::size_t block_size) : m_block_size(block_size) {}

    void *CommandQueue::Arena::allocate(std::size_t size, std::size_t alignment)
    {
        while (m_block_index < m_blocks.size()) {
            Block &block = m_blocks[m_block_index];
            const auto base = reinterpret_cast<std::uintptr_t>(block.data.get());
            const std::uintptr_t aligned = (base + m_offset + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
            const std::size_t end = static_cast<std::size_t>(aligned - base) + size;

            if (end <= block.size) {
                m_offset = end;
                return reinterpret_cast<void *>(aligned);
            }

            ++m_block_index;
            m_offset = 0;
        }

        const std::size_t block_size = std::max(m_block_size, size + alignment);
        m_blocks.push_back({ std::make_unique<std::byte[]>(block_size), block_size });
        m_block_index = m_blocks.size() - 1;
        m_offset = 0;
        return allocate(size, alignment);
    }

    void CommandQueue::Arena::reset() noexcept
    {
        m_block_index = 0;
        m_offset = 0;
    }

    CommandQueue::CommandQueue(Mode mode, std::size_t block_size) :
        m_mode(mode), m_block_size(block_size), m_recording(std::make_unique<Arena>(block_size)), m_executing(std::make_unique<Arena>(block_size))
    {
    }

    CommandQueue::~CommandQueue()
    {
        destroyPending();
    }

    void CommandQueue::flush()
    {
        while (!m_commands.empty()) {
            std::vector<Command> batch;
            batch.swap(m_commands);
            std::swap(m_recording, m_executing);

            std::size_t index = 0;
            try {
                for (; index < batch.size(); ++index) {
                    Execute(batch[index].invoke, batch[index].object, batch[index].location);
                    batch[index].destroy(batch[index].object);
                }
            } catch (...) {
                for (; index < batch.size(); ++index) batch[index].destroy(batch[index].object);
                m_executing->reset();
                throw;
            }

            m_executing->reset();
        }
    }

    CommandQueue::Batch::~Batch()
    {
        destroy();
    }

    CommandQueue::Batch::Batch(Batch &&other) noexcept : m_arena(std::move(other.m_arena)), m_commands(std::move(other.m_commands))
    {
        other.m_commands.clear();
    }

    CommandQueue::Batch &CommandQueue::Batch::operator=(Batch &&other) noexcept
    {
        if (this != &other) {
            destroy();
            m_arena = std::move(other.m_arena);
            m_commands = std::move(other.m_commands);
            other.m_commands.clear();
        }
        return *this;
    }

    void CommandQueue::Batch::execute()
    {
        std::size_t index = 0;
        try {
            for (; index < m_commands.size(); ++index) {
                CommandQueue::Execute(m_commands[index].invoke, m_commands[index].object, m_commands[index].location);
                m_commands[index].destroy(m_commands[index].object);
            }
        } catch (...) {
            for (; index < m_commands.size(); ++index) m_commands[index].destroy(m_commands[index].object);
            m_commands.clear();
            throw;
        }
        m_commands.clear();
    }

    std::size_t CommandQueue::Batch::size() const noexcept
    {
        return m_commands.size();
    }

    void CommandQueue::Batch::destroy() noexcept
    {
        for (Command &command : m_commands) command.destroy(command.object);
        m_commands.clear();
    }

    CommandQueue::Batch CommandQueue::take()
    {
        Batch batch;
        batch.m_commands.swap(m_commands);
        batch.m_arena = std::move(m_recording);
        m_recording = std::make_unique<Arena>(m_block_size);
        return batch;
    }

    std::size_t CommandQueue::pendingCount() const noexcept
    {
        return m_commands.size();
    }

    CommandQueue::Mode CommandQueue::mode() const noexcept
    {
        return m_mode;
    }

    void CommandQueue::setMode(Mode mode)
    {
        if (mode == Mode::Inline)
            flush();
        m_mode = mode;
    }

    void CommandQueue::Execute(void (*invoke)(void *), void *object, const std::source_location &location)
    {
        try {
            invoke(object);
        } catch (const std::exception &e) {
            spdlog::error("Rendering command from {}:{} failed: {}", location.file_name(), location.line(), e.what());
        }
    }

    void CommandQueue::destroyPending() noexcept
    {
        for (Command &command : m_commands) command.destroy(command.object);
        m_commands.clear();
        m_recording->reset();
    }
} // namespace atmo::server::rendering
