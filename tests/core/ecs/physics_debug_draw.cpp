#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "project/project_manager.hpp"
#include "server/rendering/rendering_server.hpp"

namespace
{
    bool HasPhysicsDebugDraw(bool editor_isolated)
    {
        atmo::server::rendering::RenderingServer::Instance();
        flecs::world world;
        world.init_builtin_components();
        atmo::core::components::register_core_components(world);
        world.set<atmo::core::components::WorldContext>({ .is_editor_isolated = editor_isolated });
        atmo::core::ecs::EntityRegistry::RegisterSystemsForWorld(&world);
        return world.lookup("Scene_DebugDraw2d").is_valid();
    }
} // namespace

TEST_CASE("Physics debug draw follows the editor and in-game project settings", "[physics]")
{
    auto &debug = atmo::project::ProjectManager::GetSettings().debug;
    const auto saved = debug;

    REQUIRE(atmo::project::Debug{}.draw_physics_debug);
    REQUIRE_FALSE(atmo::project::Debug{}.draw_physics_debug_in_game);

    debug = atmo::project::Debug{};
    CHECK(HasPhysicsDebugDraw(true));
    CHECK_FALSE(HasPhysicsDebugDraw(false));

    debug.draw_physics_debug = false;
    debug.draw_physics_debug_in_game = true;
    CHECK_FALSE(HasPhysicsDebugDraw(true));
    CHECK(HasPhysicsDebugDraw(false));

    debug = saved;
}
