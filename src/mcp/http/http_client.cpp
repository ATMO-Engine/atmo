#include "http_client.hpp"

#include <algorithm>
#include <mutex>
#include <string_view>
#include <utility>

#include "curl/curl.h"

namespace atmo::mcp::http
{
    struct HttpClient::Transfer {
        ClientRequest request;
        ClientCallback done;
        ClientResponse response;
        CURL *easy = nullptr;
        curl_slist *headers = nullptr;
        char error[CURL_ERROR_SIZE]{};

        ~Transfer()
        {
            if (headers)
                curl_slist_free_all(headers);
            if (easy)
                curl_easy_cleanup(easy);
        }

        void finish()
        {
            if (done)
                done(std::move(response));
        }
    };

    namespace
    {
        CURLM *Multi(void *multi)
        {
            return static_cast<CURLM *>(multi);
        }

        std::string_view Trim(std::string_view text)
        {
            while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
            while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r' || text.back() == '\n')) text.remove_suffix(1);
            return text;
        }

        std::size_t WriteBody(char *data, std::size_t size, std::size_t count, void *user)
        {
            static_cast<ClientResponse *>(user)->body.append(data, size * count);
            return size * count;
        }

        std::size_t WriteHeader(char *data, std::size_t size, std::size_t count, void *user)
        {
            auto *response = static_cast<ClientResponse *>(user);
            const std::string_view line(data, size * count);
            if (line.starts_with("HTTP/")) {
                response->headers.clear();
            } else if (const auto colon = line.find(':'); colon != std::string_view::npos) {
                response->headers.emplace_back(std::string(Trim(line.substr(0, colon))), std::string(Trim(line.substr(colon + 1))));
            }
            return size * count;
        }
    } // namespace

    std::string ClientResponse::header(std::string_view name) const
    {
        return std::string(FindHeader(headers, name).value_or(std::string_view{}));
    }

    HttpClient::HttpClient()
    {
        static std::once_flag once;
        std::call_once(once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
        m_multi = curl_multi_init();
    }

    HttpClient::~HttpClient()
    {
        std::lock_guard lock(m_step_mutex);
        for (auto &transfer : m_active) curl_multi_remove_handle(Multi(m_multi), transfer->easy);
        m_active.clear();
        {
            std::lock_guard pending(m_pending_mutex);
            m_pending.clear();
        }
        if (m_multi)
            curl_multi_cleanup(Multi(m_multi));
    }

    void HttpClient::request(ClientRequest request, ClientCallback done)
    {
        auto transfer = std::make_unique<Transfer>();
        transfer->request = std::move(request);
        transfer->done = std::move(done);
        std::lock_guard lock(m_pending_mutex);
        m_pending.push_back(std::move(transfer));
    }

    bool HttpClient::idle() const
    {
        std::scoped_lock lock(m_step_mutex, m_pending_mutex);
        return m_active.empty() && m_pending.empty();
    }

    void HttpClient::start(std::unique_ptr<Transfer> transfer, std::vector<std::unique_ptr<Transfer>> &finished)
    {
        Transfer &t = *transfer;
        t.easy = curl_easy_init();
        if (!t.easy || !m_multi) {
            t.response.error = "Could not create an HTTP transfer";
            finished.push_back(std::move(transfer));
            return;
        }

        CURL *easy = t.easy;
        curl_easy_setopt(easy, CURLOPT_URL, t.request.url.c_str());
        curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(t.request.connect_timeout.count()));
        curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, static_cast<long>(t.request.timeout.count()));
        curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, t.error);
        curl_easy_setopt(easy, CURLOPT_PRIVATE, &t);
        curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, &WriteBody);
        curl_easy_setopt(easy, CURLOPT_WRITEDATA, &t.response);
        curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, &WriteHeader);
        curl_easy_setopt(easy, CURLOPT_HEADERDATA, &t.response);
        if (!t.request.use_proxy)
            curl_easy_setopt(easy, CURLOPT_PROXY, "");

        if (t.request.method == "HEAD") {
            curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
        } else if (t.request.method != "GET" || !t.request.body.empty()) {
            if (t.request.method == "POST")
                curl_easy_setopt(easy, CURLOPT_POST, 1L);
            else
                curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, t.request.method.c_str());
            curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(t.request.body.size()));
            curl_easy_setopt(easy, CURLOPT_POSTFIELDS, t.request.body.data());
        }

        for (const auto &[name, value] : t.request.headers) t.headers = curl_slist_append(t.headers, (name + ": " + value).c_str());
        t.headers = curl_slist_append(t.headers, "Expect:");
        curl_easy_setopt(easy, CURLOPT_HTTPHEADER, t.headers);

        if (curl_multi_add_handle(Multi(m_multi), easy) != CURLM_OK) {
            t.response.error = "Could not start the HTTP transfer";
            finished.push_back(std::move(transfer));
            return;
        }
        m_active.push_back(std::move(transfer));
    }

    void HttpClient::step()
    {
        std::vector<std::unique_ptr<Transfer>> finished;
        {
            std::lock_guard lock(m_step_mutex);

            std::vector<std::unique_ptr<Transfer>> pending;
            {
                std::lock_guard pending_lock(m_pending_mutex);
                pending.swap(m_pending);
            }
            for (auto &transfer : pending) start(std::move(transfer), finished);

            if (!m_active.empty()) {
                int running = 0;
                curl_multi_perform(Multi(m_multi), &running);

                int left = 0;
                while (CURLMsg *message = curl_multi_info_read(Multi(m_multi), &left)) {
                    if (message->msg != CURLMSG_DONE)
                        continue;
                    CURL *easy = message->easy_handle;
                    const CURLcode result = message->data.result;

                    auto it = std::ranges::find_if(m_active, [easy](const auto &transfer) { return transfer->easy == easy; });
                    if (it == m_active.end())
                        continue;
                    Transfer &t = **it;
                    curl_multi_remove_handle(Multi(m_multi), easy);
                    if (result == CURLE_OK)
                        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &t.response.status);
                    else
                        t.response.error = t.error[0] != '\0' ? std::string(t.error) : std::string(curl_easy_strerror(result));
                    finished.push_back(std::move(*it));
                    m_active.erase(it);
                }
            }
        }

        for (auto &transfer : finished) transfer->finish();
    }

    void HttpClient::cancelAll(const std::string &reason)
    {
        std::vector<std::unique_ptr<Transfer>> cancelled;
        {
            std::lock_guard lock(m_step_mutex);
            for (auto &transfer : m_active) {
                curl_multi_remove_handle(Multi(m_multi), transfer->easy);
                cancelled.push_back(std::move(transfer));
            }
            m_active.clear();
            std::lock_guard pending_lock(m_pending_mutex);
            for (auto &transfer : m_pending) cancelled.push_back(std::move(transfer));
            m_pending.clear();
        }
        for (auto &transfer : cancelled) {
            transfer->response.error = reason;
            transfer->finish();
        }
    }
} // namespace atmo::mcp::http
