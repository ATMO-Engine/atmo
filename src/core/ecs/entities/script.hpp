#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core/resource/loaders/script_loader.hpp"
#include "core/resource/resource_manager.hpp"
#include "core/resource/resource_ref.hpp"
#include "luau/script_instance.hpp"
#include "meta/component_meta.hpp"

namespace atmo::core::components
{
    struct Script {
        std::string script_path;
        std::string prev_script_path;
        std::unique_ptr<resource::ResourceRef<resource::Bytecode>> m_res;
        atmo::luau::ScriptInstance *instance = nullptr;
        std::uint32_t physics_event_id = 0;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Script> {
    static constexpr const char *name = "Script";
    static constexpr const char *category = "Logic";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::Script::script_path>("script_path").withWidget("file_path"));
};
