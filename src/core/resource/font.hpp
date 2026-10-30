#pragma once

#include "server/rendering/rid.hpp"

namespace atmo::core::resource
{
    class Font
    {
    public:
        explicit Font(server::rendering::FontRid rid) noexcept;
        ~Font();

        Font(const Font &) = delete;
        Font &operator=(const Font &) = delete;

        [[nodiscard]] server::rendering::FontRid rid() const noexcept;

    private:
        server::rendering::FontRid m_rid;
    };
} // namespace atmo::core::resource
