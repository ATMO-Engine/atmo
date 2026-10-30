#pragma once

#include <memory>
#include <string>

#include "core/resource/resource.hpp"
#include "core/resource/texture_2d.hpp"

namespace atmo::core::resource
{
    class Texture2dLoader : public Resource<Texture2d>
    {
    public:
        std::shared_ptr<Texture2d> load(const std::string &path) override;
    };
} // namespace atmo::core::resource
