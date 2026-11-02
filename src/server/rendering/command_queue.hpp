#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <source_location>
#include <type_traits>
#include <utility>
#include <vector>

namespace atmo::server::rendering
{
    class CommandQueue
    {
    private:
        class Arena;
        struct Command;

    public:
        enum class Mode : std::uint8_t {
            Deferred,
            Inline,
        };

        explicit CommandQueue(Mode mode = Mode::Deferred, std::size_t block_size = 64 * 1024);
        ~CommandQueue();

        CommandQueue(const CommandQueue &) = delete;
        CommandQueue &operator=(const CommandQueue &) = delete;

        template <typename F>
            requires std::invocable<std::decay_t<F> &>
        void push(F &&fn, std::source_location location = std::source_location::current())
        {
            using Fn = std::decay_t<F>;

            if (m_mode == Mode::Inline) {
                Fn local(std::forward<F>(fn));
                Execute(&Invoke<Fn>, &local, location);
                return;
            }

            void *memory = m_recording->allocate(sizeof(Fn), alignof(Fn));
            Fn *object = ::new (memory) Fn(std::forward<F>(fn));
            m_commands.push_back({ &Invoke<Fn>, &Destroy<Fn>, object, location });
        }

        class Batch
        {
        public:
            Batch() = default;
            ~Batch();

            Batch(Batch &&other) noexcept;
            Batch &operator=(Batch &&other) noexcept;

            Batch(const Batch &) = delete;
            Batch &operator=(const Batch &) = delete;

            void execute();
            [[nodiscard]] std::size_t size() const noexcept;

        private:
            friend class CommandQueue;

            void destroy() noexcept;

            std::unique_ptr<Arena> m_arena;
            std::vector<Command> m_commands;
        };

        void flush();
        [[nodiscard]] Batch take();

        [[nodiscard]] std::size_t pendingCount() const noexcept;
        [[nodiscard]] Mode mode() const noexcept;
        void setMode(Mode mode);

    private:
        class Arena
        {
        public:
            explicit Arena(std::size_t block_size);

            void *allocate(std::size_t size, std::size_t alignment);
            void reset() noexcept;

        private:
            struct Block {
                std::unique_ptr<std::byte[]> data;
                std::size_t size = 0;
            };

            std::vector<Block> m_blocks;
            std::size_t m_block_index = 0;
            std::size_t m_offset = 0;
            std::size_t m_block_size;
        };

        struct Command {
            void (*invoke)(void *) = nullptr;
            void (*destroy)(void *) noexcept = nullptr;
            void *object = nullptr;
            std::source_location location;
        };

        template <typename Fn> static void Invoke(void *object)
        {
            (*static_cast<Fn *>(object))();
        }

        template <typename Fn> static void Destroy(void *object) noexcept
        {
            static_cast<Fn *>(object)->~Fn();
        }

        static void Execute(void (*invoke)(void *), void *object, const std::source_location &location);

        void destroyPending() noexcept;

        Mode m_mode;
        std::size_t m_block_size;
        std::unique_ptr<Arena> m_recording;
        std::unique_ptr<Arena> m_executing;
        std::vector<Command> m_commands;
    };
} // namespace atmo::server::rendering
