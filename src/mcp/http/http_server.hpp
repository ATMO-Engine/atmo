#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "mcp/http/http_types.hpp"
#include "mcp/http/socket.hpp"

namespace atmo::mcp::http
{
    struct HttpRequest {
        std::string method;
        std::string target;
        std::string path;
        Headers headers;
        std::string body;

        [[nodiscard]] std::string header(std::string_view name) const;
    };

    struct HttpResponse {
        int status = 200;
        Headers headers;
        std::string body;
    };

    class HttpResponder
    {
    public:
        void send(HttpResponse response) const;

    private:
        friend class HttpServer;

        struct Slot {
            std::mutex mutex;
            std::optional<HttpResponse> response;
            bool answered = false;
        };

        explicit HttpResponder(std::shared_ptr<Slot> slot);

        std::shared_ptr<Slot> m_slot;
    };

    using HttpHandler = std::function<void(const HttpRequest &request, const HttpResponder &responder)>;

    class HttpServer
    {
    public:
        explicit HttpServer(HttpHandler handler);
        ~HttpServer();

        HttpServer(const HttpServer &) = delete;
        HttpServer &operator=(const HttpServer &) = delete;

        bool listen(const std::string &host, int port);
        void step();
        void close();

        [[nodiscard]] int port() const
        {
            return m_port;
        }

        void setMaxBodySize(std::size_t size)
        {
            m_max_body = size;
        }

        void setIoTimeout(std::chrono::milliseconds timeout)
        {
            m_io_timeout = timeout;
        }

        struct Connection;

    private:
        void advance(Connection &connection, std::chrono::steady_clock::time_point now);
        void dispatch(Connection &connection);

        HttpHandler m_handler;
        std::optional<Socket> m_listener;
        int m_port = 0;
        std::vector<std::unique_ptr<Connection>> m_connections;
        std::size_t m_max_body = 16 * 1024 * 1024;
        std::chrono::milliseconds m_io_timeout{ 10000 };
    };
} // namespace atmo::mcp::http
