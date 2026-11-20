#include "socket.hpp"

#include <algorithm>
#include <climits>
#include <mutex>
#include <utility>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace atmo::mcp::http
{
    namespace
    {
#if defined(_WIN32)
        using Native = SOCKET;
        const Native invalid_native = INVALID_SOCKET;

        void EnsureInitialized()
        {
            static std::once_flag once;
            std::call_once(once, [] {
                WSADATA data;
                WSAStartup(MAKEWORD(2, 2), &data);
            });
        }

        bool LastErrorWouldBlock()
        {
            const int error = WSAGetLastError();
            return error == WSAEWOULDBLOCK || error == WSAEINTR;
        }

        void CloseNative(Native socket)
        {
            closesocket(socket);
        }

        bool Configure(Native socket)
        {
            SetHandleInformation(reinterpret_cast<HANDLE>(socket), HANDLE_FLAG_INHERIT, 0);
            u_long non_blocking = 1;
            return ioctlsocket(socket, FIONBIO, &non_blocking) == 0;
        }
#else
        using Native = int;
        const Native invalid_native = -1;

        void EnsureInitialized() {}

        bool LastErrorWouldBlock()
        {
            return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
        }

        void CloseNative(Native socket)
        {
            ::close(socket);
        }

        bool Configure(Native socket)
        {
            fcntl(socket, F_SETFD, FD_CLOEXEC);
#if defined(SO_NOSIGPIPE)
            int yes = 1;
            setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
#endif
            const int flags = fcntl(socket, F_GETFL, 0);
            return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
        }
#endif

        Native ToNative(std::intptr_t handle)
        {
            return static_cast<Native>(handle);
        }

        std::intptr_t FromNative(Native socket)
        {
            return static_cast<std::intptr_t>(socket);
        }

        Socket::IoResult Result(long long count)
        {
            if (count > 0)
                return { Socket::Status::Done, static_cast<std::size_t>(count) };
            if (count == 0)
                return { Socket::Status::Closed, 0 };
            return { LastErrorWouldBlock() ? Socket::Status::WouldBlock : Socket::Status::Failed, 0 };
        }
    } // namespace

    Socket::Socket(std::intptr_t handle) : m_handle(handle) {}

    Socket::~Socket()
    {
        close();
    }

    Socket::Socket(Socket &&other) noexcept : m_handle(std::exchange(other.m_handle, FromNative(invalid_native))) {}

    Socket &Socket::operator=(Socket &&other) noexcept
    {
        if (this != &other) {
            close();
            m_handle = std::exchange(other.m_handle, FromNative(invalid_native));
        }
        return *this;
    }

    std::optional<Socket> Socket::Listen(const std::string &host, int port, int &bound_port)
    {
        EnsureInitialized();

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

        addrinfo *addresses = nullptr;
        const std::string service = std::to_string(port);
        if (getaddrinfo(host.empty() ? nullptr : host.c_str(), service.c_str(), &hints, &addresses) != 0)
            return std::nullopt;

        std::optional<Socket> listener;
        for (addrinfo *address = addresses; address && !listener; address = address->ai_next) {
            const Native native = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
            if (native == invalid_native)
                continue;
            Socket socket(FromNative(native));

#if defined(_WIN32)
            BOOL exclusive = TRUE;
            setsockopt(native, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char *>(&exclusive), sizeof(exclusive));
#else
            int reuse = 1;
            setsockopt(native, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

            if (!Configure(native) || ::bind(native, address->ai_addr, static_cast<socklen_t>(address->ai_addrlen)) != 0 || ::listen(native, SOMAXCONN) != 0)
                continue;

            sockaddr_storage bound{};
            socklen_t length = sizeof(bound);
            if (getsockname(native, reinterpret_cast<sockaddr *>(&bound), &length) != 0)
                continue;
            if (bound.ss_family == AF_INET6)
                bound_port = ntohs(reinterpret_cast<const sockaddr_in6 *>(&bound)->sin6_port);
            else
                bound_port = ntohs(reinterpret_cast<const sockaddr_in *>(&bound)->sin_port);

            listener = std::move(socket);
        }

        freeaddrinfo(addresses);
        return listener;
    }

    std::optional<Socket> Socket::accept() const
    {
        if (!valid())
            return std::nullopt;
        const Native native = ::accept(ToNative(m_handle), nullptr, nullptr);
        if (native == invalid_native)
            return std::nullopt;
        Socket socket(FromNative(native));
        if (!Configure(native))
            return std::nullopt;
        return socket;
    }

    Socket::IoResult Socket::receive(char *data, std::size_t size) const
    {
#if defined(_WIN32)
        return Result(::recv(ToNative(m_handle), data, static_cast<int>(std::min<std::size_t>(size, INT_MAX)), 0));
#else
        return Result(::recv(ToNative(m_handle), data, size, 0));
#endif
    }

    Socket::IoResult Socket::send(const char *data, std::size_t size) const
    {
        if (size == 0)
            return { Status::Done, 0 };
#if defined(_WIN32)
        const long long sent = ::send(ToNative(m_handle), data, static_cast<int>(std::min<std::size_t>(size, INT_MAX)), 0);
#elif defined(MSG_NOSIGNAL)
        const long long sent = ::send(ToNative(m_handle), data, size, MSG_NOSIGNAL);
#else
        const long long sent = ::send(ToNative(m_handle), data, size, 0);
#endif
        if (sent == 0)
            return { Status::WouldBlock, 0 };
        return Result(sent);
    }

    void Socket::shutdownWrite() const
    {
        if (!valid())
            return;
#if defined(_WIN32)
        ::shutdown(ToNative(m_handle), SD_SEND);
#else
        ::shutdown(ToNative(m_handle), SHUT_WR);
#endif
    }

    void Socket::close()
    {
        if (valid())
            CloseNative(ToNative(m_handle));
        m_handle = FromNative(invalid_native);
    }

    bool Socket::valid() const
    {
        return ToNative(m_handle) != invalid_native;
    }
} // namespace atmo::mcp::http
