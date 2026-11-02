#include "font.hpp"

#include "server/rendering/rendering_server.hpp"

namespace atmo::core::resource
{
    Font::Font(server::rendering::FontRid rid) noexcept : m_rid(rid) {}

    Font::~Font()
    {
        server::rendering::RenderingServer::Instance().fontFree(m_rid);
    }

    server::rendering::FontRid Font::rid() const noexcept
    {
        return m_rid;
    }
} // namespace atmo::core::resource
