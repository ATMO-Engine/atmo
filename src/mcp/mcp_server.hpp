#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/thread/thread_pool.hpp"
#include "glaze/glaze.hpp"
#include "mcp/tool.hpp"

namespace httplib
{
    class Server;
    struct Request;
    struct Response;
} // namespace httplib

namespace atmo::core
{
    class Engine;
} // namespace atmo::core

namespace atmo::mcp
{
    class McpServer
    {
    public:
        enum class Mode {
            Editor,
            Game,
        };

        static constexpr std::string_view latest_protocol_version = "2025-11-25";

        McpServer(core::Engine &engine, Mode mode);
        ~McpServer();

        McpServer(const McpServer &) = delete;
        McpServer &operator=(const McpServer &) = delete;

        bool start(const std::string &host, int port);
        void stop();

        [[nodiscard]] int port() const
        {
            return m_port;
        }

        [[nodiscard]] Mode mode() const
        {
            return m_mode;
        }

        [[nodiscard]] core::Engine &engine() const
        {
            return m_engine;
        }

        ToolRegistry &tools()
        {
            return m_tools;
        }

        void setTimeout(std::chrono::milliseconds timeout)
        {
            m_timeout = timeout;
        }

        std::optional<std::string> handleMessage(std::string_view body, bool *initialize = nullptr);

        void after(std::uint64_t frames, std::function<void()> fn);
        void tick();

    private:
        glz::generic dispatch(const glz::generic &request, bool *initialize);
        glz::generic callTool(const glz::generic &params, const glz::generic &id);
        void handleHttp(const httplib::Request &request, httplib::Response &response);
        void failInflight(const std::string &message);

        core::Engine &m_engine;
        Mode m_mode;
        ToolRegistry m_tools;
        std::chrono::milliseconds m_timeout{ 30000 };
        int m_port = 0;

        std::unique_ptr<httplib::Server> m_http;
        std::optional<core::TaskHandle> m_listen_task;
        std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);

        std::mutex m_inflight_mutex;
        std::list<Reply> m_inflight;

        std::mutex m_session_mutex;
        std::set<std::string> m_sessions;

        std::vector<std::pair<std::uint64_t, std::function<void()>>> m_scheduled;
        std::uint64_t m_ticks = 0;
        bool m_in_tick = false;
        std::uint32_t m_tick_callback = 0;
        bool m_has_tick_callback = false;
    };
} // namespace atmo::mcp
