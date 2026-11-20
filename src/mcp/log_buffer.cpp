#include "log_buffer.hpp"

#include <chrono>
#include <deque>
#include <format>
#include <memory>
#include <mutex>

#include "spdlog/sinks/base_sink.h"
#include "spdlog/spdlog.h"

namespace atmo::mcp
{
    namespace
    {
        struct StoredEntry {
            std::uint64_t seq;
            spdlog::level::level_enum level;
            std::string time;
            std::string message;
        };

        class BufferSink final : public spdlog::sinks::base_sink<std::mutex>
        {
        public:
            std::vector<LogBuffer::Entry> read(std::uint64_t since, spdlog::level::level_enum min_level, std::size_t limit)
            {
                std::lock_guard lock(mutex_);
                std::vector<LogBuffer::Entry> entries;
                for (const auto &entry : m_entries) {
                    if (entry.seq <= since || entry.level < min_level)
                        continue;
                    const auto level_name = spdlog::level::to_string_view(entry.level);
                    entries.push_back({ entry.seq, entry.time, std::string(level_name.data(), level_name.size()), entry.message });
                }
                if (entries.size() > limit)
                    entries.erase(entries.begin(), entries.end() - static_cast<std::ptrdiff_t>(limit));
                return entries;
            }

            std::uint64_t last()
            {
                std::lock_guard lock(mutex_);
                return m_next - 1;
            }

        protected:
            void sink_it_(const spdlog::details::log_msg &msg) override
            {
                const auto time = std::chrono::floor<std::chrono::milliseconds>(msg.time);
                m_entries.push_back({ m_next++, msg.level, std::format("{:%T}", time), std::string(msg.payload.begin(), msg.payload.end()) });
                if (m_entries.size() > LogBuffer::capacity)
                    m_entries.pop_front();
            }

            void flush_() override {}

        private:
            std::deque<StoredEntry> m_entries;
            std::uint64_t m_next = 1;
        };

        std::shared_ptr<BufferSink> &Sink()
        {
            static std::shared_ptr<BufferSink> sink;
            return sink;
        }
    } // namespace

    void LogBuffer::Install()
    {
        if (Sink())
            return;
        Sink() = std::make_shared<BufferSink>();
        Sink()->set_level(spdlog::level::trace);
        spdlog::default_logger()->sinks().push_back(Sink());
    }

    std::vector<LogBuffer::Entry> LogBuffer::Read(std::uint64_t since, spdlog::level::level_enum min_level, std::size_t limit)
    {
        if (!Sink())
            return {};
        return Sink()->read(since, min_level, limit);
    }

    std::uint64_t LogBuffer::LastSequence()
    {
        if (!Sink())
            return 0;
        return Sink()->last();
    }
} // namespace atmo::mcp
