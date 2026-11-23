#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace atmo::mcp::http
{
    class Socket
    {
    public:
        enum class Status {
            Done,
            WouldBlock,
            Closed,
            Failed,
        };

        struct IoResult {
            Status status;
            std::size_t bytes;
        };

        Socket() = default;
        ~Socket();

        Socket(Socket &&other) noexcept;
        Socket &operator=(Socket &&other) noexcept;
        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;

        static std::optional<Socket> Listen(const std::string &host, int port, int &bound_port);

        std::optional<Socket> accept() const;
        IoResult receive(char *data, std::size_t size) const;
        IoResult send(const char *data, std::size_t size) const;
        void shutdownWrite() const;
        void close();

        [[nodiscard]] bool valid() const;

    private:
        explicit Socket(std::intptr_t handle);

        std::intptr_t m_handle = -1;
    };
} // namespace atmo::mcp::http
