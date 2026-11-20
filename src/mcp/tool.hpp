#pragma once

#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "glaze/glaze.hpp"
#include "mcp/schema.hpp"

namespace atmo::mcp
{
    struct ToolResult {
        glz::generic::array_t content;
        bool is_error = false;

        static ToolResult Text(std::string text);
        static ToolResult Json(const glz::generic &value);
        static ToolResult Error(std::string message);
        static ToolResult Image(std::string base64_data, std::string mime_type);

        ToolResult &addText(std::string text);

        [[nodiscard]] glz::generic toJson() const;
    };

    class Reply
    {
    public:
        Reply();

        void send(ToolResult result) const;

        void error(std::string message) const
        {
            send(ToolResult::Error(std::move(message)));
        }

        void json(const glz::generic &value) const
        {
            send(ToolResult::Json(value));
        }

        void text(std::string text) const
        {
            send(ToolResult::Text(std::move(text)));
        }

        [[nodiscard]] bool sent() const;
        [[nodiscard]] std::optional<ToolResult> wait(std::chrono::milliseconds timeout) const;

    private:
        struct State {
            std::mutex mutex;
            std::condition_variable cv;
            std::optional<ToolResult> result;
        };

        std::shared_ptr<State> m_state;
    };

    using ToolHandler = std::function<void(const glz::generic &arguments, const Reply &reply)>;

    struct Tool {
        std::string name;
        std::string description;
        glz::generic input_schema;
        bool main_thread = true;
        ToolHandler handler;
    };

    struct NoArgs {
    };

    inline constexpr glz::opts strict_args_opts{ .error_on_unknown_keys = true, .error_on_missing_keys = true };

    class ToolRegistry
    {
    public:
        template <class Args, class Fn> void add(std::string name, std::string description, Fn fn, bool main_thread = true)
        {
            Tool tool;
            tool.name = std::move(name);
            tool.description = std::move(description);
            tool.input_schema = SchemaFor<Args>();
            tool.main_thread = main_thread;
            tool.handler = [fn = std::move(fn)](const glz::generic &arguments, const Reply &reply) {
                Args args{};
                std::string json = arguments.is_null() ? std::string("{}") : arguments.dump().value_or("{}");
                if (auto ec = glz::read<strict_args_opts>(args, json)) {
                    reply.error("Invalid arguments: " + glz::format_error(ec, json));
                    return;
                }
                try {
                    fn(args, reply);
                } catch (const std::exception &e) {
                    reply.error(e.what());
                }
            };
            addTool(std::move(tool));
        }

        void addTool(Tool tool);

        [[nodiscard]] const Tool *find(std::string_view name) const;

        [[nodiscard]] const std::vector<Tool> &all() const
        {
            return m_tools;
        }

    private:
        std::vector<Tool> m_tools;
    };
} // namespace atmo::mcp

template <> struct glz::meta<atmo::mcp::NoArgs> {
    static constexpr auto value = glz::object();
};
