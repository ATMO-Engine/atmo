#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/thread/thread_pool.hpp"
#include "glaze/glaze.hpp"
#include "mcp/tool.hpp"

namespace atmo::mcp::http
{
    class HttpClient;
    class HttpServer;
    class HttpResponder;
    struct HttpRequest;
} // namespace atmo::mcp::http

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

        using MessageCallback = std::function<void(std::optional<std::string> response)>;

        void handleMessage(std::string_view body, MessageCallback done, bool *initialize = nullptr);

        [[nodiscard]] std::shared_ptr<http::HttpClient> httpClient() const
        {
            return m_client;
        }

        void after(std::uint64_t frames, std::function<void()> fn);
        void tick();

    private:
        using Respond = std::function<void(glz::generic response)>;

        struct Inflight {
            Reply reply;
            std::string tool;
            std::chrono::steady_clock::time_point deadline;
        };

        void dispatch(const glz::generic &request, bool *initialize, const Respond &respond);
        void callTool(const glz::generic &params, const glz::generic &id, const Respond &respond);
        void handleHttp(const http::HttpRequest &request, const http::HttpResponder &responder);
        void poll();
        void expireInflight();
        void failInflight(const std::string &message);

        core::Engine &m_engine;
        Mode m_mode;
        ToolRegistry m_tools;
        std::chrono::milliseconds m_timeout{ 30000 };
        int m_port = 0;

        std::unique_ptr<http::HttpServer> m_http;
        std::shared_ptr<http::HttpClient> m_client;
        std::optional<core::TaskHandle> m_poll_task;
        std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);

        std::mutex m_inflight_mutex;
        std::unordered_map<std::uint64_t, Inflight> m_inflight;
        std::uint64_t m_next_inflight = 0;

        std::mutex m_session_mutex;
        std::set<std::string> m_sessions;

        std::vector<std::pair<std::uint64_t, std::function<void()>>> m_scheduled;
        std::uint64_t m_ticks = 0;
        bool m_in_tick = false;
        std::uint32_t m_tick_callback = 0;
        bool m_has_tick_callback = false;
    };
} // namespace atmo::mcp
