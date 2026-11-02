#pragma once

#include <memory>
#include <string>

#include "core/resource/material.hpp"
#include "core/resource/resource.hpp"

namespace atmo::core::resource
{
    class MaterialLoader : public Resource<Material>
    {
    public:
        std::shared_ptr<Material> load(const std::string &path) override;
    };
} // namespace atmo::core::resource
