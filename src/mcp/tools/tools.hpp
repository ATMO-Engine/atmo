#pragma once

namespace atmo::mcp
{
    class McpServer;

    void RegisterTools(McpServer &server);

    namespace tools
    {
        void RegisterCommonTools(McpServer &server);
        void RegisterSceneTools(McpServer &server);
        void RegisterEditorTools(McpServer &server);
        void RegisterProjectTools(McpServer &server);
        void RegisterRenderTools(McpServer &server);
        void RegisterGameTools(McpServer &server);
        void RegisterPlayTools(McpServer &server);
    } // namespace tools
} // namespace atmo::mcp
