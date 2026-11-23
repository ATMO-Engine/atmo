#include "tools.hpp"

#include "mcp/mcp_server.hpp"

namespace atmo::mcp
{
    void RegisterTools(McpServer &server)
    {
        tools::RegisterCommonTools(server);
        tools::RegisterSceneTools(server);
        tools::RegisterRenderTools(server);
        tools::RegisterProjectTools(server);
        if (server.mode() == McpServer::Mode::Editor) {
            tools::RegisterEditorTools(server);
            tools::RegisterPlayTools(server);
        } else {
            tools::RegisterGameTools(server);
        }
    }
} // namespace atmo::mcp
