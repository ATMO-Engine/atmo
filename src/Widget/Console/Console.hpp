#include "spdlog/spdlog.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "../Widget.hpp"

class Console : public Widget
{
    public:
        Console();
        ~Console() = default;

        void run() override;

    protected:
        const std::string widgetName = "Logs";

    private:
        // class ConsoleSink : public spdlog::sinks::sink
        // {
        //     public:
        //         ConsoleSink(const ConsoleSink& other) = delete;
        //         ConsoleSink& operator=(const ConsoleSink& other) = delete;

        //         ~ConsoleSink() override = default;

        //         void log(const spdlog::details::log_msg &msg) override
        //         {
        //             std::string formatted = fmt::to_string(msg.payload);
        //         }
        // };

        // std::shared_ptr<spdlog::logger> logger;
        // std::shared_ptr<ConsoleSink> sink;
};
