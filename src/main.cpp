#include <csignal>
#include <cstdlib>
#include <vector>

#include "atmo.hpp"
#include "spdlog/spdlog.h"

int main(int argc, const char *const *argv)
{
    atmo::core::Engine engine;

    if (int ret = engine.init(); ret != 0)
        return ret == -1 ? 0 : 1;

    atmo::addon::AtmoAPI api{ .engine = &engine,
                              .input_manager = atmo::core::InputManager::Instance(),
                              .arg_manager = atmo::core::args::ArgManager::Instance(),
                              .entity_registry = atmo::core::ecs::EntityRegistry::Instance(),
                              .subresource_registry = atmo::core::resource::SubResourceRegistry::Instance(),
                              .meta_registry = atmo::meta::MetaRegistry::Instance() };

    std::vector<atmo::addon::Addon> addons;

    for (auto &addon_pair : atmo::project::ProjectManager::GetSettings().addons.addons) {
        if (!addon_pair.second)
            continue;

        try {
            auto addon = atmo::addon::Addon(addon_pair.first, &api);
            addons.emplace_back(addon);

            spdlog::debug("Loaded addon: {} {} ({})", addon.header.name, addon.header.version.toString(), addon.header.internal_name);
        } catch (atmo::addon::Addon::AddonLoadException &err) {
            spdlog::warn("Failed to load addon at \"{}\": {}", addon_pair.first, err.what());
        }
    }

    if (int ret = engine.args(argc, argv); ret != 0)
        return ret == -1 ? 0 : 1;

    engine.start();

    addons.clear();
    return 0;
}
