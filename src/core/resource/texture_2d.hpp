#pragma once

#include "core/types.hpp"
#include "server/rendering/rid.hpp"

namespace atmo::core::resource
{
    class Texture2d
    {
    public:
        Texture2d(server::rendering::TextureRid rid, types::Vector2i size) noexcept;
        ~Texture2d();

        Texture2d(const Texture2d &) = delete;
        Texture2d &operator=(const Texture2d &) = delete;

        [[nodiscard]] server::rendering::TextureRid rid() const noexcept;
        [[nodiscard]] types::Vector2i size() const noexcept;

    private:
        server::rendering::TextureRid m_rid;
        types::Vector2i m_size;
    };
} // namespace atmo::core::resource
