#pragma once

#include <memory>
#include <string>

#include "core/resource/font.hpp"
#include "core/resource/resource.hpp"

namespace atmo::core::resource
{
    class FontLoader : public Resource<Font>
    {
    public:
        std::shared_ptr<Font> load(const std::string &path) override;
    };
} // namespace atmo::core::resource
