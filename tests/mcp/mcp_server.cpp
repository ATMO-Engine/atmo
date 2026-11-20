#include <atomic>
#include <chrono>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/engine.hpp"
#include "core/thread/thread_pool.hpp"
#include "mcp/http/http_client.hpp"
#include "mcp/http/http_server.hpp"
#include "mcp/log_buffer.hpp"
#include "mcp/mcp_server.hpp"
#include "mcp/schema.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

using atmo::mcp::McpServer;
using namespace std::chrono_literals;

namespace mcp_test
{
    struct SchemaArgs {
        std::string required_text;
        std::optional<int> optional_number;
        glz::generic required_value;
        std::optional<glz::generic> optional_value;
        std::vector<std::string> names;
    };
} // namespace mcp_test

namespace
{
    atmo::core::Engine &TestEngine()
    {
        atmo::server::rendering::RenderingServer::Instance();
        static atmo::core::Engine engine;
        static const bool initialized = [] {
            engine.reset();
            auto scene = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::Scene>("Entity::Scene");
            engine.getECS().changeScene(scene);
            return true;
        }();
        (void)initialized;
        return engine;
    }

    glz::generic Parse(const std::optional<std::string> &text)
    {
        REQUIRE(text.has_value());
        auto json = glz::read_json<glz::generic>(*text);
        REQUIRE(json.has_value());
        return *json;
    }

    std::optional<std::string> Message(McpServer &server, const std::string &body, bool *initialize = nullptr)
    {
        std::optional<std::optional<std::string>> response;
        server.handleMessage(body, [&response](std::optional<std::string> answer) { response = std::move(answer); }, initialize);
        while (!response) {
            atmo::core::SignalQueue::Flush();
            server.tick();
            std::this_thread::sleep_for(1ms);
        }
        return *response;
    }

    glz::generic Rpc(McpServer &server, const std::string &body)
    {
        return Parse(Message(server, body));
    }

    atmo::mcp::http::ClientResponse Send(atmo::mcp::http::HttpClient &client, atmo::mcp::http::ClientRequest request, const std::function<void()> &pump = [] {})
    {
        request.use_proxy = false;
        request.timeout = 10s;
        std::optional<atmo::mcp::http::ClientResponse> response;
        client.request(std::move(request), [&response](atmo::mcp::http::ClientResponse answer) { response = std::move(answer); });
        while (!response) {
            client.step();
            pump();
            atmo::core::SignalQueue::Flush();
            std::this_thread::sleep_for(1ms);
        }
        return *response;
    }

    atmo::mcp::http::ClientResponse PostMcp(atmo::mcp::http::HttpClient &client, int port, const std::string &body, atmo::mcp::http::Headers headers = {})
    {
        atmo::mcp::http::ClientRequest request;
        request.method = "POST";
        request.url = std::format("http://127.0.0.1:{}/mcp", port);
        request.headers = std::move(headers);
        request.headers.emplace_back("Content-Type", "application/json");
        request.body = body;
        return Send(client, std::move(request));
    }

    struct CallResult {
        bool is_error = false;
        std::string text;
        glz::generic json;
    };

    CallResult Call(McpServer &server, const std::string &tool, const std::string &arguments = "{}")
    {
        glz::generic response =
            Rpc(server, std::format(R"({{"jsonrpc":"2.0","id":7,"method":"tools/call","params":{{"name":"{}","arguments":{}}}}})", tool, arguments));
        REQUIRE(response.contains("result"));
        const glz::generic &result = response["result"];
        CallResult call;
        call.is_error = result.contains("isError") && result["isError"].get_boolean();
        call.text = result["content"][0]["text"].get_string();
        call.json = glz::read_json<glz::generic>(call.text).value_or(glz::generic{});
        return call;
    }
} // namespace

TEST_CASE("MCP argument schemas list required and optional fields", "[mcp]")
{
    const glz::generic schema = atmo::mcp::SchemaFor<mcp_test::SchemaArgs>();
    REQUIRE(schema["type"].get_string() == "object");
    REQUIRE(schema["properties"]["required_text"]["type"].get_string() == "string");
    REQUIRE(schema["properties"]["optional_number"]["type"].get_string() == "integer");
    REQUIRE(schema["properties"]["names"]["type"].get_string() == "array");
    REQUIRE(schema["properties"]["names"]["items"]["type"].get_string() == "string");
    REQUIRE_FALSE(schema["properties"]["required_value"].contains("type"));

    std::vector<std::string> required;
    for (const auto &name : schema["required"].get_array()) required.push_back(name.get_string());
    REQUIRE(required == std::vector<std::string>{ "names", "required_text", "required_value" });
}

TEST_CASE("MCP initialize negotiates the protocol version", "[mcp]")
{
    McpServer server(TestEngine(), McpServer::Mode::Game);
    bool initialize = false;

    glz::generic response = Parse(Message(server, R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18"}})", &initialize));
    REQUIRE(initialize);
    REQUIRE(response["id"].get_number() == 1);
    REQUIRE(response["result"]["protocolVersion"].get_string() == "2025-06-18");
    REQUIRE(response["result"]["serverInfo"]["name"].get_string() == "atmo");
    REQUIRE(response["result"]["capabilities"].contains("tools"));

    response = Parse(Message(server, R"({"jsonrpc":"2.0","id":"a","method":"initialize","params":{"protocolVersion":"1999-01-01"}})"));
    REQUIRE(response["id"].get_string() == "a");
    REQUIRE(response["result"]["protocolVersion"].get_string() == McpServer::latest_protocol_version);

    REQUIRE_FALSE(Message(server, R"({"jsonrpc":"2.0","method":"notifications/initialized"})").has_value());
    REQUIRE(Parse(Message(server, R"({"jsonrpc":"2.0","id":2,"method":"ping"})"))["result"].is_object());
}

TEST_CASE("MCP tools/list describes every tool with an object schema", "[mcp]")
{
    McpServer server(TestEngine(), McpServer::Mode::Game);
    glz::generic response = Parse(Message(server, R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})"));
    const auto &tools = response["result"]["tools"].get_array();
    REQUIRE(tools.size() == server.tools().all().size());

    std::vector<std::string> names;
    for (const auto &tool : tools) {
        names.push_back(tool["name"].get_string());
        REQUIRE_FALSE(tool["description"].get_string().empty());
        const glz::generic &schema = tool["inputSchema"];
        REQUIRE(schema["type"].get_string() == "object");
        if (schema.contains("required"))
            for (const auto &field : schema["required"].get_array()) REQUIRE(schema["properties"].contains(field.get_string()));
    }
    for (const char *name : { "game_status", "game_step", "game_input", "scene_tree", "entity_create", "component_set", "luau_eval", "logs_read" })
        REQUIRE(std::find(names.begin(), names.end(), name) != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "editor_open") == names.end());
}

TEST_CASE("MCP reports protocol and argument errors", "[mcp]")
{
    McpServer server(TestEngine(), McpServer::Mode::Game);

    REQUIRE(Parse(Message(server, "{oops"))["error"]["code"].get_number() == -32700);
    REQUIRE(Parse(Message(server, R"({"jsonrpc":"2.0","id":1,"method":"resources/list"})"))["error"]["code"].get_number() == -32601);
    REQUIRE(Parse(Message(server, R"({"jsonrpc":"2.0","id":1})"))["error"]["code"].get_number() == -32600);
    REQUIRE(Parse(Message(server, R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"nope"}})"))["error"]["code"].get_number() == -32602);

    CallResult missing = Call(server, "entity_get", "{}");
    REQUIRE(missing.is_error);
    REQUIRE(missing.text.find("Invalid arguments") != std::string::npos);

    CallResult unknown = Call(server, "entity_get", R"({"entity":"Player","extra":1})");
    REQUIRE(unknown.is_error);
}

TEST_CASE("MCP scene tools edit the running scene", "[mcp]")
{
    McpServer server(TestEngine(), McpServer::Mode::Game);

    CallResult group = Call(server, "entity_create", R"({"type":"Entity2d","name":"McpGroup"})");
    REQUIRE_FALSE(group.is_error);
    REQUIRE(group.json["path"].get_string() == "McpGroup");

    CallResult player =
        Call(server, "entity_create", R"({"type":"Entity::Entity2d","name":"McpPlayer","components":{"Transform2d":{"position":{"x":12,"y":-4}}}})");
    REQUIRE_FALSE(player.is_error);
    REQUIRE_FALSE(player.json.contains("errors"));

    CallResult got = Call(server, "entity_get", R"({"entity":"McpPlayer"})");
    REQUIRE(got.json["components"]["Transform2d"]["position"]["x"].get_number() == 12);
    REQUIRE(got.json["components"]["Transform2d"]["scale"]["x"].get_number() == 1);

    CallResult set = Call(server, "component_set", R"({"entity":"McpPlayer","component":"Transform2d","value":{"rotation":45}})");
    REQUIRE_FALSE(set.is_error);
    REQUIRE(set.json["rotation"].get_number() == 45);
    REQUIRE(set.json["position"]["x"].get_number() == 12);

    REQUIRE(Call(server, "component_set", R"({"entity":"McpPlayer","component":"NoSuchComponent","value":{}})").is_error);

    CallResult moved = Call(server, "entity_reparent", R"({"entity":"McpPlayer","parent":"McpGroup"})");
    REQUIRE(moved.json["path"].get_string() == "McpGroup/McpPlayer");
    REQUIRE(Call(server, "entity_reparent", R"({"entity":"McpGroup","parent":"McpGroup/McpPlayer"})").is_error);

    CallResult by_id = Call(server, "entity_get", std::format(R"({{"entity":"{}"}})", static_cast<std::uint64_t>(player.json["id"].get_number())));
    REQUIRE(by_id.json["path"].get_string() == "McpGroup/McpPlayer");

    CallResult renamed = Call(server, "entity_rename", R"({"entity":"McpGroup/McpPlayer","name":"Hero"})");
    REQUIRE(renamed.json["path"].get_string() == "McpGroup/Hero");

    CallResult tree = Call(server, "scene_tree", R"({"entity":"McpGroup"})");
    REQUIRE(tree.json["children"].get_array().size() == 1);
    REQUIRE(tree.json["children"][0]["name"].get_string() == "Hero");

    REQUIRE_FALSE(Call(server, "component_remove", R"({"entity":"McpGroup/Hero","component":"Transform2d"})").is_error);
    REQUIRE_FALSE(Call(server, "entity_get", R"({"entity":"McpGroup/Hero"})").json["components"].contains("Transform2d"));

    REQUIRE_FALSE(Call(server, "entity_delete", R"({"entity":"McpGroup"})").is_error);
    REQUIRE(Call(server, "entity_get", R"({"entity":"McpGroup"})").is_error);
    REQUIRE(Call(server, "entity_delete", R"({"entity":"/"})").is_error);
}

TEST_CASE("MCP game_step pauses the game and answers after the frames", "[mcp]")
{
    auto &engine = TestEngine();
    McpServer server(engine, McpServer::Mode::Game);

    CallResult step = Call(server, "game_step", R"({"frames":3})");
    REQUIRE_FALSE(step.is_error);
    REQUIRE(step.json["paused"].get_boolean());
    REQUIRE(engine.isPaused());

    REQUIRE_FALSE(Call(server, "game_resume").json["paused"].get_boolean());
    REQUIRE_FALSE(engine.isPaused());
    REQUIRE(engine.pendingSteps() == 0);

    CallResult input = Call(server, "game_input", R"({"events":[{"type":"key_press","key":"NotAKey"}]})");
    REQUIRE(input.is_error);
}

TEST_CASE("MCP luau_eval returns results, printed lines and errors", "[mcp]")
{
    McpServer server(TestEngine(), McpServer::Mode::Game);

    CallResult ok = Call(server, "luau_eval", R"({"code":"print('hello', 2) return 1 + 2, 'x'"})");
    REQUIRE_FALSE(ok.is_error);
    REQUIRE(ok.json["results"][0].get_string() == "3");
    REQUIRE(ok.json["results"][1].get_string() == "x");
    REQUIRE(ok.json["printed"][0].get_string() == "hello\t2");

    CallResult failed = Call(server, "luau_eval", R"json({"code":"error('boom')"})json");
    REQUIRE(failed.is_error);
    REQUIRE(failed.json["error"].get_string().find("boom") != std::string::npos);

    CallResult syntax = Call(server, "luau_eval", R"({"code":"return +"})");
    REQUIRE(syntax.is_error);
}

TEST_CASE("MCP logs_read returns the entries after a sequence number", "[mcp]")
{
    atmo::mcp::LogBuffer::Install();
    McpServer server(TestEngine(), McpServer::Mode::Game);

    const std::uint64_t before = atmo::mcp::LogBuffer::LastSequence();
    spdlog::warn("mcp test marker");
    CallResult logs = Call(server, "logs_read", std::format(R"({{"since":{},"level":"warning"}})", before));
    REQUIRE_FALSE(logs.is_error);
    const auto &entries = logs.json["entries"].get_array();
    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0]["message"].get_string() == "mcp test marker");
    REQUIRE(entries[0]["level"].get_string() == "warning");
    REQUIRE(Call(server, "logs_read", R"({"level":"loud"})").is_error);
}

TEST_CASE("MCP tools that never answer time out", "[mcp]")
{
    McpServer server(TestEngine(), McpServer::Mode::Game);
    server.tools().add<atmo::mcp::NoArgs>("never_answers", "Test tool.", [](const atmo::mcp::NoArgs &, const atmo::mcp::Reply &) {});
    server.setTimeout(50ms);

    CallResult result = Call(server, "never_answers");
    REQUIRE(result.is_error);
    REQUIRE(result.text.find("did not complete") != std::string::npos);
}

TEST_CASE("SignalQueue accepts tasks from other threads", "[mcp]")
{
    std::atomic<int> ran{ 0 };
    std::vector<atmo::core::TaskHandle> tasks;
    for (int i = 0; i < 8; ++i)
        tasks.push_back(atmo::core::ThreadPool::Instance().submit([&ran] {
            for (int j = 0; j < 100; ++j) atmo::core::SignalQueue::Enqueue([&ran] { ++ran; });
        }));
    while (ran < 800) {
        atmo::core::SignalQueue::Flush();
        std::this_thread::sleep_for(1ms);
    }
    for (auto &task : tasks) task.wait();
    atmo::core::SignalQueue::Flush();
    REQUIRE(ran == 800);
}

TEST_CASE("MCP HTTP transport handles sessions and origins", "[mcp]")
{
    McpServer server(TestEngine(), McpServer::Mode::Game);
    REQUIRE(server.start("127.0.0.1", 0));
    REQUIRE(server.port() > 0);
    const int port = server.port();

    atmo::mcp::http::HttpClient client;
    auto initialized = PostMcp(client, port, R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})");
    REQUIRE(initialized.ok());
    REQUIRE(initialized.status == 200);
    REQUIRE(initialized.header("content-type") == "application/json");
    const std::string session = initialized.header("Mcp-Session-Id");
    REQUIRE_FALSE(session.empty());

    auto ping = PostMcp(client, port, R"({"jsonrpc":"2.0","id":2,"method":"ping"})", { { "Mcp-Session-Id", session } });
    REQUIRE(ping.status == 200);
    REQUIRE(Parse(ping.body)["result"].is_object());

    auto tool = PostMcp(
        client, port, R"({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"game_status","arguments":{}}})", { { "Mcp-Session-Id", session } });
    REQUIRE(tool.status == 200);
    REQUIRE_FALSE(Parse(tool.body)["result"].contains("isError"));

    auto notification = PostMcp(client, port, R"({"jsonrpc":"2.0","method":"notifications/initialized"})", { { "Mcp-Session-Id", session } });
    REQUIRE(notification.status == 202);

    auto stale = PostMcp(client, port, R"({"jsonrpc":"2.0","id":4,"method":"ping"})", { { "Mcp-Session-Id", "stale" } });
    REQUIRE(stale.status == 404);

    auto foreign = PostMcp(client, port, R"({"jsonrpc":"2.0","id":5,"method":"ping"})", { { "Origin", "http://example.com" } });
    REQUIRE(foreign.status == 403);

    auto get = Send(client, atmo::mcp::http::ClientRequest{ .url = std::format("http://127.0.0.1:{}/mcp", port) });
    REQUIRE(get.status == 405);
    REQUIRE(get.header("Allow") == "POST, DELETE");

    auto other = Send(client, atmo::mcp::http::ClientRequest{ .url = std::format("http://127.0.0.1:{}/other", port) });
    REQUIRE(other.status == 404);

    server.stop();
    REQUIRE_FALSE(PostMcp(client, port, R"({"jsonrpc":"2.0","id":6,"method":"ping"})").ok());
}

TEST_CASE("HttpServer answers a request after its handler has returned", "[mcp]")
{
    using namespace atmo::mcp::http;

    std::optional<HttpResponder> pending;
    HttpRequest seen;
    HttpServer server([&](const HttpRequest &request, const HttpResponder &responder) {
        seen = request;
        pending = responder;
    });
    REQUIRE(server.listen("127.0.0.1", 0));

    HttpClient client;
    ClientRequest request;
    request.method = "PUT";
    request.url = std::format("http://127.0.0.1:{}/echo?x=1", server.port());
    request.headers = { { "X-Test", "Value" } };
    request.body = std::string(200 * 1024, 'a');

    int steps_after_dispatch = 0;
    ClientResponse response = Send(client, request, [&] {
        server.step();
        if (pending && ++steps_after_dispatch == 20)
            pending->send(HttpResponse{ .status = 201, .headers = { { "X-Answer", "later" } }, .body = "done" });
    });

    REQUIRE(response.ok());
    REQUIRE(response.status == 201);
    REQUIRE(response.body == "done");
    REQUIRE(response.header("x-answer") == "later");
    REQUIRE(seen.method == "PUT");
    REQUIRE(seen.path == "/echo");
    REQUIRE(seen.target == "/echo?x=1");
    REQUIRE(seen.header("x-test") == "Value");
    REQUIRE(seen.body.size() == 200 * 1024);
}

TEST_CASE("The MCP server holds no pool worker while it listens and waits on a tool", "[mcp]")
{
    McpServer server(TestEngine(), McpServer::Mode::Game);
    REQUIRE(server.start("127.0.0.1", 0));

    atmo::mcp::http::HttpClient client;
    std::optional<atmo::mcp::http::ClientResponse> answer;
    atmo::mcp::http::ClientRequest request;
    request.method = "POST";
    request.url = std::format("http://127.0.0.1:{}/mcp", server.port());
    request.headers = { { "Content-Type", "application/json" } };
    request.body = R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"game_status","arguments":{}}})";
    request.use_proxy = false;
    client.request(request, [&answer](atmo::mcp::http::ClientResponse response) { answer = std::move(response); });
    for (int i = 0; i < 50; ++i) {
        client.step();
        std::this_thread::sleep_for(1ms);
    }
    REQUIRE_FALSE(answer.has_value());

    const std::size_t workers = atmo::core::ThreadPool::Instance().getWorkerCount();
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    std::atomic<std::size_t> arrived{ 0 };
    std::atomic<std::size_t> met{ 0 };
    std::vector<atmo::core::TaskHandle> tasks;
    for (std::size_t i = 0; i < workers; ++i)
        tasks.push_back(atmo::core::ThreadPool::Instance().submit([&] {
            ++arrived;
            while (arrived < workers && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
            if (arrived >= workers)
                ++met;
        }));
    for (auto &task : tasks) task.wait();
    REQUIRE(met == workers);

    while (!answer) {
        client.step();
        atmo::core::SignalQueue::Flush();
        std::this_thread::sleep_for(1ms);
    }
    REQUIRE(answer->status == 200);
}
