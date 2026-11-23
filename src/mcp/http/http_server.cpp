#include "http_server.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <format>
#include <string_view>
#include <utility>

#include "llhttp.h"

namespace atmo::mcp::http
{
    namespace
    {
        constexpr std::size_t max_header_bytes = 64 * 1024;
        constexpr std::size_t max_accepts_per_step = 16;
        constexpr std::size_t max_reads_per_step = 64;

        std::string_view ReasonPhrase(int status)
        {
            switch (status) {
                case 100:
                    return "Continue";
                case 200:
                    return "OK";
                case 202:
                    return "Accepted";
                case 400:
                    return "Bad Request";
                case 403:
                    return "Forbidden";
                case 404:
                    return "Not Found";
                case 405:
                    return "Method Not Allowed";
                case 408:
                    return "Request Timeout";
                case 413:
                    return "Content Too Large";
                case 431:
                    return "Request Header Fields Too Large";
                case 500:
                    return "Internal Server Error";
                default:
                    return "";
            }
        }

        std::string Serialize(const HttpResponse &response)
        {
            std::string out = std::format("HTTP/1.1 {} {}\r\n", response.status, ReasonPhrase(response.status));
            for (const auto &[name, value] : response.headers) out += std::format("{}: {}\r\n", name, value);
            out += std::format("Content-Length: {}\r\nConnection: close\r\n\r\n", response.body.size());
            out += response.body;
            return out;
        }

        bool EqualsIgnoreCase(std::string_view a, std::string_view b)
        {
            return std::ranges::equal(
                a, b, [](char x, char y) { return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y)); });
        }
    } // namespace

    struct HttpServer::Connection {
        enum class State {
            Reading,
            Waiting,
            Writing,
            Closed,
        };

        Socket socket;
        llhttp_t parser{};
        State state = State::Reading;
        HttpRequest request;
        std::string field;
        std::string value;
        std::size_t header_bytes = 0;
        std::size_t max_body = 0;
        int error_status = 0;
        bool expect_continue = false;
        std::chrono::steady_clock::time_point deadline;
        std::shared_ptr<HttpResponder::Slot> slot;
        std::string out;
        std::size_t out_offset = 0;

        static Connection &From(llhttp_t *parser)
        {
            return *static_cast<Connection *>(parser->data);
        }

        int appendHeaderBytes(std::string &target, const char *at, std::size_t length)
        {
            header_bytes += length;
            if (header_bytes > max_header_bytes) {
                error_status = 431;
                return -1;
            }
            target.append(at, length);
            return 0;
        }

        void respond(int status)
        {
            out += Serialize(HttpResponse{ .status = status, .headers = {}, .body = {} });
            state = State::Writing;
        }
    };

    namespace
    {
        const llhttp_settings_t &ParserSettings()
        {
            static const llhttp_settings_t settings = [] {
                using Connection = HttpServer::Connection;
                llhttp_settings_t s;
                llhttp_settings_init(&s);
                s.on_url = [](llhttp_t *parser, const char *at, std::size_t length) {
                    Connection &c = Connection::From(parser);
                    return c.appendHeaderBytes(c.request.target, at, length);
                };
                s.on_header_field = [](llhttp_t *parser, const char *at, std::size_t length) {
                    Connection &c = Connection::From(parser);
                    return c.appendHeaderBytes(c.field, at, length);
                };
                s.on_header_value = [](llhttp_t *parser, const char *at, std::size_t length) {
                    Connection &c = Connection::From(parser);
                    return c.appendHeaderBytes(c.value, at, length);
                };
                s.on_header_value_complete = [](llhttp_t *parser) {
                    Connection &c = Connection::From(parser);
                    c.request.headers.emplace_back(std::move(c.field), std::move(c.value));
                    c.field.clear();
                    c.value.clear();
                    return 0;
                };
                s.on_headers_complete = [](llhttp_t *parser) {
                    Connection &c = Connection::From(parser);
                    if (parser->content_length > c.max_body && (parser->flags & F_CONTENT_LENGTH)) {
                        c.error_status = 413;
                        return -1;
                    }
                    auto expect = FindHeader(c.request.headers, "Expect");
                    c.expect_continue = expect && EqualsIgnoreCase(*expect, "100-continue");
                    return 0;
                };
                s.on_body = [](llhttp_t *parser, const char *at, std::size_t length) {
                    Connection &c = Connection::From(parser);
                    if (c.request.body.size() + length > c.max_body) {
                        c.error_status = 413;
                        return -1;
                    }
                    c.request.body.append(at, length);
                    return 0;
                };
                s.on_message_complete = [](llhttp_t *parser) {
                    Connection &c = Connection::From(parser);
                    c.request.method = llhttp_method_name(static_cast<llhttp_method_t>(llhttp_get_method(parser)));
                    return static_cast<int>(HPE_PAUSED);
                };
                return s;
            }();
            return settings;
        }
    } // namespace

    std::string HttpRequest::header(std::string_view name) const
    {
        return std::string(FindHeader(headers, name).value_or(std::string_view{}));
    }

    HttpResponder::HttpResponder(std::shared_ptr<Slot> slot) : m_slot(std::move(slot)) {}

    void HttpResponder::send(HttpResponse response) const
    {
        std::lock_guard lock(m_slot->mutex);
        if (m_slot->answered)
            return;
        m_slot->answered = true;
        m_slot->response = std::move(response);
    }

    HttpServer::HttpServer(HttpHandler handler) : m_handler(std::move(handler)) {}

    HttpServer::~HttpServer()
    {
        close();
    }

    bool HttpServer::listen(const std::string &host, int port)
    {
        close();
        int bound = 0;
        m_listener = Socket::Listen(host, port, bound);
        if (!m_listener)
            return false;
        m_port = bound;
        return true;
    }

    void HttpServer::close()
    {
        m_connections.clear();
        m_listener.reset();
    }

    void HttpServer::step()
    {
        if (!m_listener)
            return;

        const auto now = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < max_accepts_per_step; ++i) {
            std::optional<Socket> socket = m_listener->accept();
            if (!socket)
                break;
            auto connection = std::make_unique<Connection>();
            connection->socket = std::move(*socket);
            connection->max_body = m_max_body;
            connection->deadline = now + m_io_timeout;
            llhttp_init(&connection->parser, HTTP_REQUEST, &ParserSettings());
            connection->parser.data = connection.get();
            m_connections.push_back(std::move(connection));
        }

        for (auto &connection : m_connections) advance(*connection, now);
        std::erase_if(m_connections, [](const auto &connection) { return connection->state == Connection::State::Closed; });
    }

    void HttpServer::advance(Connection &c, std::chrono::steady_clock::time_point now)
    {
        if (c.state == Connection::State::Reading) {
            char buffer[16 * 1024];
            for (std::size_t i = 0; i < max_reads_per_step && c.state == Connection::State::Reading; ++i) {
                const Socket::IoResult read = c.socket.receive(buffer, sizeof(buffer));
                if (read.status == Socket::Status::WouldBlock)
                    break;
                if (read.status != Socket::Status::Done) {
                    c.state = Connection::State::Closed;
                    return;
                }

                const bool expected_continue = c.expect_continue;
                const llhttp_errno_t error = llhttp_execute(&c.parser, buffer, read.bytes);
                if (error == HPE_PAUSED) {
                    c.state = Connection::State::Waiting;
                    dispatch(c);
                } else if (error != HPE_OK) {
                    c.respond(c.error_status != 0 ? c.error_status : 400);
                } else if (c.expect_continue && !expected_continue) {
                    c.out += "HTTP/1.1 100 Continue\r\n\r\n";
                }
            }
            if (c.state == Connection::State::Reading && now >= c.deadline)
                c.respond(408);
        }

        if (c.state == Connection::State::Waiting) {
            std::optional<HttpResponse> response;
            {
                std::lock_guard lock(c.slot->mutex);
                response = std::exchange(c.slot->response, std::nullopt);
            }
            if (response) {
                c.out += Serialize(*response);
                c.state = Connection::State::Writing;
                c.deadline = now + m_io_timeout;
            } else {
                char scratch[512];
                const Socket::IoResult read = c.socket.receive(scratch, sizeof(scratch));
                if (read.status == Socket::Status::Closed || read.status == Socket::Status::Failed) {
                    c.state = Connection::State::Closed;
                    return;
                }
            }
        }

        while (c.out_offset < c.out.size()) {
            const Socket::IoResult written = c.socket.send(c.out.data() + c.out_offset, c.out.size() - c.out_offset);
            if (written.status == Socket::Status::WouldBlock)
                break;
            if (written.status != Socket::Status::Done) {
                c.state = Connection::State::Closed;
                return;
            }
            c.out_offset += written.bytes;
        }

        if (c.out_offset == c.out.size()) {
            c.out.clear();
            c.out_offset = 0;
            if (c.state == Connection::State::Writing) {
                c.socket.shutdownWrite();
                c.state = Connection::State::Closed;
            }
        } else if (c.state == Connection::State::Writing && now >= c.deadline) {
            c.state = Connection::State::Closed;
        }
    }

    void HttpServer::dispatch(Connection &c)
    {
        c.request.path = c.request.target.substr(0, c.request.target.find('?'));
        c.slot = std::make_shared<HttpResponder::Slot>();
        const HttpResponder responder(c.slot);
        try {
            m_handler(c.request, responder);
        } catch (const std::exception &) {
            responder.send(HttpResponse{ .status = 500, .headers = {}, .body = {} });
        }
    }
} // namespace atmo::mcp::http
