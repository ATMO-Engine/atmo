#include "ui_number_input.hpp"
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "ctre.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UINumberInput::RegisterSystems(flecs::world *world)
    {
        world->system<core::components::UINumberInput>("NumberInput_Update")
            .kind(flecs::OnStore)
            .each([world](flecs::entity e, core::components::UINumberInput &comp) {
                spdlog::info("LOUTRE update : {}", std::get<float>(comp.value));
                if (comp.value != comp.prev_value) {
                    auto wrapped = EntityRegistry::Wrap(e);
                    auto *ui = dynamic_cast<entities::UIInput *>(wrapped.get());

                    comp.prev_value = comp.value;
                    if (ui->getComponent<core::components::UIInput>().input_type == core::components::UIInput::InputType::Int)
                        ui->getComponentMutable<core::components::UIInput>().input_data = std::to_string(std::get<int>(comp.value));

                    if (ui->getComponent<core::components::UIInput>().input_type == core::components::UIInput::InputType::Float) {
                        ui->getComponentMutable<core::components::UIInput>().input_data = std::to_string(std::get<float>(comp.value));
                        spdlog::info("number input update comp value: {}", std::get<float>(comp.value));
                        spdlog::info("number input update input value: {}", ui->getComponentMutable<core::components::UIInput>().input_data);
                    }
                }
            });
    };

    void UINumberInput::validateInput()
    {
        auto &input_content = getComponentMutable<core::components::UIInput>();
        auto &entity_content = getComponentMutable<core::components::UINumberInput>();

        switch (input_content.input_type) {
            case core::components::UIInput::InputType::Int:
                {
                    if (ctre::match<"^-?[0-9]+$">(input_content.input_data)) {
                        int value;

                        auto [ptr, ec] =
                            std::from_chars(input_content.input_data.data(), input_content.input_data.data() + input_content.input_data.size(), value);

                        if (ec == std::errc()) {
                            entity_content.value = value;
                        } else {
                            clear();
                        }
                    } else {
                        clear();
                    }
                    break;
                }

            case core::components::UIInput::InputType::Float:
                {
                    if (ctre::match<"^-?[0-9]+(\\.[0-9]+)?$">(input_content.input_data)) {
                        try {
                            entity_content.value = std::stof(input_content.input_data);
                        } catch (...) {
                            clear();
                        }
                    } else {
                        clear();
                    }
                    break;
                }
            default:
                return;
        }
    };

    void UINumberInput::clear()
    {
        auto &input_content = getComponentMutable<core::components::UIInput>();
        auto &entity_content = getComponentMutable<core::components::UINumberInput>();

        switch (input_content.input_type) {
            case core::components::UIInput::InputType::Int:
                {
                    input_content.input_data.clear();
                    input_content.input_data = "0";
                    entity_content.value = 0;
                    entity_content.prev_value = 0;
                    break;
                }
            case core::components::UIInput::InputType::Float:
                {
                    input_content.input_data.clear();
                    input_content.input_data = "0.0";
                    entity_content.value = 0.0f;
                    entity_content.prev_value = 0.0f;
                    break;
                }
            default:
                break;
        }
    };

    Clay_ElementDeclaration UINumberInput::buildDecl()
    {
        return UI::buildDecl();
    }


    void UINumberInput::initialize()
    {
        UIInput::initialize();
        setComponent<core::components::UINumberInput>({});

        auto button = getChildren()[0];
        auto label = UILabel(button.getChildren()[0]);
        auto &input_comp = getComponentMutable<core::components::UIInput>();

        input_comp.input_type = core::components::UIInput::InputType::Float;
        clear();
        label.setText(input_comp.input_data);
    }

    void UINumberInput::draw(ClaySdL3RendererData *data)
    {
        UIInput::draw(data);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UINumberInput);
