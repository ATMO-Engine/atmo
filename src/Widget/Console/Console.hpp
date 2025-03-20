#include <iostream>
#include <memory>
#include "../Widget.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "spdlog/spdlog.h"

class Console : public Widget
{
    private:
        class ConsoleSink : public spdlog::sinks::sink
        {
            public:
                ConsoleSink();
                ConsoleSink(const ConsoleSink &other) = delete;
                ConsoleSink &operator=(const ConsoleSink &other) = delete;

                ~ConsoleSink() override = default;

                void log(const spdlog::details::log_msg &msg) override
                {
                    std::string formatted;
                    spdlog::memory_buf_t formattedMsg;
                    formatter->format(msg, formattedMsg);
                    formatted = std::string(formattedMsg.data(), formattedMsg.size());
                    std::lock_guard<std::mutex> lock(mutex);
                    logs.push_back(formatted);
                }

                void flush() override
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    for (const auto &log : logs) {
                        flushedLogs.push_back(log);
                    }
                    logs.clear();
                }

                void set_pattern(const std::string &pattern) override
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    this->pattern = pattern;
                }

                void set_formatter(std::unique_ptr<spdlog::formatter> sink) override
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    this->formatter = std::move(sink);
                }

                const std::vector<std::string> &getFlushedLogs()
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    return flushedLogs;
                }

                double getFlushedLogsSize()
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    double size = 0;
                    for (const auto &log : flushedLogs) {
                        size += log.size();
                    }
                    return size / 1024;
                }

                void clearFlushedLogs()
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    flushedLogs.clear();
                }

            private:
                std::vector<std::string> logs;
                std::vector<std::string> flushedLogs;
                std::mutex mutex;
                std::string pattern;
                std::unique_ptr<spdlog::formatter> formatter;
        };

    public:
        Console();
        ~Console() = default;

        void run() override;

    protected:
        const std::string widgetName = "Logs";
        std::shared_ptr<ConsoleSink> sink;
        bool autoScroll = true;
};
