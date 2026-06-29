#include "ui_text_input.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"

namespace atmo::core::ecs::entities
{
    void UITextInput::RegisterSystems(flecs::world *world)
    {
        world->system<core::components::UITextInput>("TextInput_Update")
            .kind(flecs::OnStore)
            .each([world](flecs::entity e, core::components::UITextInput &comp) {
                if (comp.value != comp.prev_value) {
                    auto wrapped = EntityRegistry::Wrap(e);
                    auto *ui = dynamic_cast<entities::UIInput *>(wrapped.get());

                    comp.prev_value = comp.value;

                    ui->getComponentMutable<core::components::UIInput>().input_data = comp.value;
                }
            });
    }

    void UITextInput::validateInput()
    {
        auto &input = getComponentMutable<core::components::UIInput>();
        auto &text = getComponentMutable<core::components::UITextInput>();

        constexpr size_t MAX_LENGTH = 15;

        if (input.input_data.size() > MAX_LENGTH) {
            input.input_data.resize(MAX_LENGTH);
        }

        text.value = input.input_data;
    };

    void UITextInput::clear()
    {
        getComponentMutable<core::components::UIInput>().input_data.clear();
        getComponentMutable<core::components::UITextInput>().value.clear();
    };

    void UITextInput::initialize()
    {
        UIInput::initialize();

        setComponent<core::components::UITextInput>({});

        auto &input_type = getComponentMutable<core::components::UIInput>();
        input_type.input_type = core::components::UIInput::InputType::Text;
    }

    Clay_ElementDeclaration UITextInput::buildDecl()
    {
        return UI::buildDecl();
    }

    void UITextInput::draw(ClaySdL3RendererData *data)
    {
        UIInput::draw(data);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UITextInput);
