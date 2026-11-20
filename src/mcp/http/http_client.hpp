#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "mcp/http/http_types.hpp"

namespace atmo::mcp::http
{
    struct ClientRequest {
        std::string method = "GET";
        std::string url;
        Headers headers;
        std::string body;
        std::chrono::milliseconds connect_timeout{ 5000 };
        std::chrono::milliseconds timeout{ 30000 };
        bool use_proxy = true;
    };

    struct ClientResponse {
        long status = 0;
        Headers headers;
        std::string body;
        std::string error;

        [[nodiscard]] bool ok() const
        {
            return error.empty();
        }

        [[nodiscard]] std::string header(std::string_view name) const;
    };

    using ClientCallback = std::function<void(ClientResponse response)>;

    class HttpClient
    {
    public:
        HttpClient();
        ~HttpClient();

        HttpClient(const HttpClient &) = delete;
        HttpClient &operator=(const HttpClient &) = delete;

        void request(ClientRequest request, ClientCallback done);
        void step();
        void cancelAll(const std::string &reason);

        [[nodiscard]] bool idle() const;

    private:
        struct Transfer;

        void start(std::unique_ptr<Transfer> transfer, std::vector<std::unique_ptr<Transfer>> &finished);

        void *m_multi = nullptr;

        mutable std::mutex m_pending_mutex;
        std::vector<std::unique_ptr<Transfer>> m_pending;

        mutable std::mutex m_step_mutex;
        std::vector<std::unique_ptr<Transfer>> m_active;
    };
} // namespace atmo::mcp::http
