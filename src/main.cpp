#include "atmo.hpp"
#include "core/components.hpp"
#include "impl/window.hpp"

#include <csignal>

atmo::core::Engine engine;

int main(int argc, char **argv)
{
    std::signal(SIGINT, [](int signum) { engine.stop(); });
    std::signal(SIGTERM, [](int signum) { engine.stop(); });

    ECS_IMPORT(engine.get_ecs(), FlecsMeta);

    atmo::core::InputManager::instance().addEvent(
        "#INTERNAL#ui_click", new atmo::core::InputManager::MouseButtonEvent(SDL_BUTTON_LEFT));

    atmo::core::InputManager::instance().addEvent(
        "#INTERNAL#ui_scroll", new atmo::core::InputManager::MouseScrollEvent());

    auto window = engine.instantiate_prefab("window", "MainWindow");
    atmo::impl::WindowManager *wm =
        static_cast<atmo::impl::WindowManager *>(window.get_ref<atmo::core::ComponentManager::Managed>()->ptr);
    wm->rename("Atmo Engine");
    wm->make_main();

    // auto window2 = engine.instantiate_prefab("window", "SubWindow");

    // auto rect = engine.get_ecs()
    //                 .entity("Rect")
    //                 .child_of(window)
    //                 .set<atmo::core::components::UIRect>({ { 0.0f, 0.5f, 0.5f, 1.0f },
    //                                                        { 10.0f, 10.0f, 10.0f, 10.0f },
    //                                                        { 2, 2, 2, 2 },
    //                                                        { 1.0f, 1.0f, 1.0f, 1.0f } })
    //                 .set<atmo::core::components::UITransform>({ 150.0f, 150.0f, 200.0f, 200.0f, 0.0f, true, true })
    //                 .set<atmo::core::components::UIStack>(
    //                     { atmo::core::components::UILayoutDirection::Horizontal, {}, {}, false, 0 });

    // auto subrect =
    //     engine.get_ecs()
    //         .entity()
    //         .child_of(rect)
    //         .set<atmo::core::components::UIRect>(
    //             { { 0.5f, 0.0f, 0.5f, 1.0f }, { 5.0f, 5.0f, 5.0f, 5.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f, 1.0f, 1.0f }
    //             })
    //         .set<atmo::core::components::UITransform>({ 0.0f, 0.0f, 100.0f, 100.0f, 0.0f, true, false });
    // auto subrect2 =
    //     engine.get_ecs()
    //         .entity()
    //         .child_of(rect)
    //         .set<atmo::core::components::UIRect>(
    //             { { 0.5f, 0.0f, 0.5f, 1.0f }, { 5.0f, 5.0f, 5.0f, 5.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f, 1.0f, 1.0f }
    //             })
    //         .set<atmo::core::components::UITransform>({ 0.0f, 0.0f, 100.0f, 100.0f, 0.0f, true, false });
    // auto subrect3 =
    //     engine.get_ecs()
    //         .entity()
    //         .child_of(rect)
    //         .set<atmo::core::components::UIRect>(
    //             { { 0.5f, 0.0f, 0.5f, 1.0f }, { 5.0f, 5.0f, 5.0f, 5.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f, 1.0f, 1.0f }
    //             })
    //         .set<atmo::core::components::UITransform>({ 0.0f, 0.0f, 100.0f, 100.0f, 0.0f, true, false });

    while (engine.get_ecs().progress()) {
        atmo::core::InputManager::instance().tick();
    }

    return 0;
}
