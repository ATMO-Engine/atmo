#include "texture_2d.hpp"

#include "server/rendering/rendering_server.hpp"

namespace atmo::core::resource
{
    Texture2d::Texture2d(server::rendering::TextureRid rid, types::Vector2i size) noexcept : m_rid(rid), m_size(size) {}

    Texture2d::~Texture2d()
    {
        server::rendering::RenderingServer::Instance().textureFree(m_rid);
    }

    server::rendering::TextureRid Texture2d::rid() const noexcept
    {
        return m_rid;
    }

    types::Vector2i Texture2d::size() const noexcept
    {
        return m_size;
    }
} // namespace atmo::core::resource
