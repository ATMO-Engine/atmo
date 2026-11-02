#pragma once

#include <memory>
#include <string>

#include "core/resource/resource.hpp"
#include "core/resource/shader.hpp"

namespace atmo::core::resource
{
    class ShaderLoader : public Resource<Shader>
    {
    public:
        std::shared_ptr<Shader> load(const std::string &path) override;
    };
} // namespace atmo::core::resource
