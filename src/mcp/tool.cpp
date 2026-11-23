#include "tool.hpp"

#include <algorithm>
#include <utility>

namespace atmo::mcp
{
    ToolResult ToolResult::Text(std::string text)
    {
        ToolResult result;
        result.addText(std::move(text));
        return result;
    }

    ToolResult ToolResult::Json(const glz::generic &value)
    {
        return Text(value.dump().value_or("null"));
    }

    ToolResult ToolResult::Error(std::string message)
    {
        ToolResult result = Text(std::move(message));
        result.is_error = true;
        return result;
    }

    ToolResult ToolResult::Image(std::string base64_data, std::string mime_type)
    {
        ToolResult result;
        result.content.push_back(glz::generic::object_t{ { "type", "image" }, { "data", std::move(base64_data) }, { "mimeType", std::move(mime_type) } });
        return result;
    }

    ToolResult &ToolResult::addText(std::string text)
    {
        content.push_back(glz::generic::object_t{ { "type", "text" }, { "text", std::move(text) } });
        return *this;
    }

    glz::generic ToolResult::toJson() const
    {
        glz::generic::object_t result{ { "content", content } };
        if (is_error)
            result["isError"] = true;
        return result;
    }

    Reply::Reply() : m_state(std::make_shared<State>()) {}

    void Reply::send(ToolResult result) const
    {
        std::function<void(const ToolResult &)> callback;
        {
            std::lock_guard lock(m_state->mutex);
            if (m_state->result)
                return;
            m_state->result = std::move(result);
            callback = std::exchange(m_state->callback, nullptr);
        }
        if (callback)
            callback(*m_state->result);
    }

    bool Reply::sent() const
    {
        std::lock_guard lock(m_state->mutex);
        return m_state->result.has_value();
    }

    void Reply::onSent(std::function<void(const ToolResult &result)> callback) const
    {
        {
            std::lock_guard lock(m_state->mutex);
            if (!m_state->result) {
                m_state->callback = std::move(callback);
                return;
            }
        }
        callback(*m_state->result);
    }

    void ToolRegistry::addTool(Tool tool)
    {
        auto it = std::find_if(m_tools.begin(), m_tools.end(), [&](const Tool &existing) { return existing.name == tool.name; });
        if (it != m_tools.end())
            *it = std::move(tool);
        else
            m_tools.push_back(std::move(tool));
    }

    const Tool *ToolRegistry::find(std::string_view name) const
    {
        auto it = std::find_if(m_tools.begin(), m_tools.end(), [&](const Tool &tool) { return tool.name == name; });
        return it == m_tools.end() ? nullptr : &*it;
    }
} // namespace atmo::mcp
