#include "ui_input.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/input/input_manager.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIInput::RegisterSystems(flecs::world *world)
    {
        world->system<core::components::UIInput>("Input_Update").kind(flecs::OnUpdate).each([world](flecs::entity e, core::components::UIInput &comp) {
            if (!comp.editing)
                return;

            auto text = InputManager::ConsumeText();

            if (!text.empty()) {
                comp.input_data += text;
            }
        });
    }

    void UIInput::initialize()
    {
        UI::initialize();

        setComponent<core::components::UIInput>({});

        auto &input_comp = getComponentMutable<core::components::UIInput>();
        auto input_rect = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &input_rect_comp = input_rect->getComponentMutable<core::components::UIRect>();
        auto &input_rect_layout = input_rect->getComponentMutable<core::components::Layout>();

        input_rect_comp.color.a = 0.0f;
        input_rect_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        input_rect_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        input_rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        input_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        input_rect->setParent(*this);

        flecs::entity test = p_handle;

        input_rect->getSignal<>("Pressed").connect([input_rect, test] {
            auto window = input_rect->getWindow()->getComponent<core::components::Window>().window;
            auto wrapped = EntityRegistry::Wrap(test);
            auto *ui = dynamic_cast<entities::UIInput *>(wrapped.get());

            test.world().each([](core::components::UIInput &input) { input.editing = false; });
            ui->getComponentMutable<core::components::UIInput>().editing = true;

            if (window) {
                core::InputManager::StartTextInput(window);
            }
        });
    }

    void UIInput::draw(ClaySdL3RendererData *data)
    {
        auto &input_comp = getComponentMutable<core::components::UIInput>();
        auto button = UIButton(getChildren()[0]);
        auto &button_comp = button.getComponentMutable<core::components::UIRect>();
        auto label = UILabel(button.getChildren()[0]);
        auto window = getWindow()->getComponent<core::components::Window>().window;

        if (!input_comp.editing) {
            button_comp.color = core::types::Color::WHITE;
            button_comp.color.a = 0.0f;
            return;
        } else {
            button_comp.color = core::types::Color::BLACK;
            button_comp.color.a = 0.3f;
        }

        if (InputManager::IsJustPressed("ui_confirm")) {
            validateInput();
            input_comp.editing = false;
            InputManager::StopTextInput(window);
        }
        if (InputManager::IsJustPressed("ui_delete") && input_comp.input_data.size() > 0) {
            input_comp.input_data = input_comp.input_data.substr(0, input_comp.input_data.size() - 1);
        }
        label.setText(input_comp.input_data);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIInput);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIInput)
