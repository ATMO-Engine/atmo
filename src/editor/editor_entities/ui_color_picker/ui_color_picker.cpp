#include "ui_color_picker.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entities/ui/ui_slider/ui_slider.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UIColorPicker::RegisterSystems(flecs::world *world) {}

    void UIColorPicker::setupRow(const std::string &labelText, const std::string &rowName)
    {
        auto color_panel = core::ecs::EntityRegistry::Create<UIRect>("Entity::UI::UIRect");
        auto &color_panel_rect = color_panel->getComponentMutable<core::components::UIRect>();
        color_panel_rect.corner_radius.top_left = 5.0f;
        color_panel_rect.corner_radius.top_right = 5.0f;
        color_panel_rect.corner_radius.bottom_left = 5.0f;
        color_panel_rect.corner_radius.bottom_right = 5.0f;
        color_panel_rect.color = core::types::Color("#7d7d7d");

        auto &color_panel_layout = color_panel->getComponentMutable<core::components::Layout>();
        color_panel_layout.direction = core::components::Layout::Direction::Horizontal;
        color_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        color_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        color_panel_layout.padding.left = 4;
        color_panel_layout.padding.right = 4;
        color_panel_layout.padding.top = 4;
        color_panel_layout.padding.bottom = 4;
        color_panel_layout.child_gap = 4;
        color_panel_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        color_panel->rename(rowName + "panel");
        color_panel->setParent(*this);

        auto colorName = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
        colorName->setText(labelText);
        colorName->setFontSize(16);
        colorName->rename(rowName + "label");
        colorName->setParent(*color_panel);

        auto slider = core::ecs::EntityRegistry::Create<UISlider>("Entity::UI::UIRect::UISlider");
        slider->setType(core::components::UISlider::SliderType::Float, 0.0f, 1.0f);
        auto &slider_layout = slider->getComponentMutable<core::components::Layout>();
        slider_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        slider_layout.width.size = 0.7f;
        slider_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        slider_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 16.0f, 16.0f };
        slider->rename(rowName + "slider");
        slider->setParent(*color_panel);

        auto numberInput = core::ecs::EntityRegistry::Create<UINumberInput>("Entity::UI::UIInput::UINumberInput");
        auto &input_rect_layout = numberInput->getComponentMutable<core::components::Layout>();
        input_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        input_rect_layout.width.size = 0.1f;

        auto &input_entity_comp = numberInput->getComponentMutable<core::components::UIInput>();
        input_entity_comp.input_type = atmo::core::components::UIInput::InputType::Float;
        input_entity_comp.input_data = "1.0";

        auto button = UIButton(numberInput->getChildren()[0]);
        if (!button.getHandle().is_alive()) {
            return;
        }
        auto label = UILabel(button.getChildren()[0]);
        if (!label.getHandle().is_alive()) {
            return;
        }
        numberInput->validateInput();
        auto &nbInput_comp = numberInput->getComponentMutable<core::components::UIInput>();
        nbInput_comp.input_data = std::format("{:g}", 1.0f);
        label.setText(nbInput_comp.input_data);

        numberInput->rename(rowName + "input");
        numberInput->setParent(*color_panel);
    }

    void UIColorPicker::initialize()
    {
        UIRect::initialize();

        setComponent<components::UIColorPicker>({});

        createSignal<types::Color>("ColorChanged");
        createSignal<types::Color>("Update preview");

        auto &rect = getComponentMutable<core::components::UIRect>();
        rect.color = core::types::Color("#acacac");

        auto &layout = getComponentMutable<core::components::Layout>();
        layout.direction = core::components::Layout::Direction::Vertical;
        layout.child_gap = 6;
        layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        layout.padding.left = 4;
        layout.padding.right = 4;
        layout.padding.top = 4;
        layout.padding.bottom = 4;

        auto color_preview_background = core::ecs::EntityRegistry::Create<UIRect>("Entity::UI::UIRect");
        auto &color_preview_background_layout = color_preview_background->getComponentMutable<core::components::Layout>();
        color_preview_background_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        color_preview_background_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        color_preview_background_layout.height.size = 0.1f;
        auto &color_preview_background_rect = color_preview_background->getComponentMutable<core::components::UIRect>();
        color_preview_background_rect.corner_radius.top_left = 5.0f;
        color_preview_background_rect.corner_radius.top_right = 5.0f;
        color_preview_background_rect.corner_radius.bottom_left = 5.0f;
        color_preview_background_rect.corner_radius.bottom_right = 5.0f;
        color_preview_background_rect.color = types::Color::WHITE;
        color_preview_background->setParent(*this);

        auto color_preview = core::ecs::EntityRegistry::Create<UIRect>("Entity::UI::UIRect");
        auto &color_preview_layout = color_preview->getComponentMutable<core::components::Layout>();
        color_preview_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        color_preview_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        auto &color_preview_rect = color_preview->getComponentMutable<core::components::UIRect>();
        color_preview_rect.corner_radius.top_left = 5.0f;
        color_preview_rect.corner_radius.top_right = 5.0f;
        color_preview_rect.corner_radius.bottom_left = 5.0f;
        color_preview_rect.corner_radius.bottom_right = 5.0f;
        color_preview->setParent(*color_preview_background);

        auto colorHandle = color_preview->getHandle();
        getSignal<types::Color>("Update preview").connect([colorHandle](types::Color color) {
            if (!colorHandle.is_alive()) {
                return;
            }

            UIRect colorEntity(core::ecs::EntityRegistry::GetEntityFromId(colorHandle));
            auto &color_rect = colorEntity.getComponentMutable<core::components::UIRect>();
            color_rect.color = color;
        });


        auto handle = p_handle;
        { /* Red slider */
            setupRow("R", std::string(RowR));

            auto color_panel = getChild(std::string(RowR) + "panel");

            auto slider = color_panel.getChild(std::string(RowR) + "slider");
            auto sliderHandle = slider.getHandle();

            auto numberInput = color_panel.getChild(std::string(RowR) + "input");
            auto numberHandle = numberInput.getHandle();

            slider.getSignal<float>("FloatValueChanged").connect([handle, numberHandle](float val) {
                if (!handle.is_alive() || !numberHandle.is_alive()) {
                    return;
                }
                core::ecs::entities::UINumberInput nbInput(core::ecs::EntityRegistry::GetEntityFromId(numberHandle));
                auto &nbInput_comp = nbInput.getComponentMutable<core::components::UINumberInput>();
                nbInput_comp.value = val;

                UIColorPicker colorPicker(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = colorPicker.getComponentMutable<core::components::UIColorPicker>();
                colorPicker.setColor({ val, comp.current_color.g, comp.current_color.b, comp.current_color.a });
            });
            numberInput.getSignal<float>("FloatValueChanged").connect([handle, sliderHandle](float val) {
                if (!handle.is_alive() || !sliderHandle.is_alive()) {
                    return;
                }
                UISlider slider(core::ecs::EntityRegistry::GetEntityFromId(sliderHandle));
                slider.setValue(val, false);

                UIColorPicker colorPicker(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = colorPicker.getComponentMutable<core::components::UIColorPicker>();
                colorPicker.setColor({ val, comp.current_color.g, comp.current_color.b, comp.current_color.a });
            });

            numberInput.getSignal<float>("FloatValueChanged").emit(1.0f);
        }
        { /* Green slider */
            setupRow("G", std::string(RowG));

            auto color_panel = getChild(std::string(RowG) + "panel");

            auto slider = color_panel.getChild(std::string(RowG) + "slider");
            auto sliderHandle = slider.getHandle();

            auto numberInput = color_panel.getChild(std::string(RowG) + "input");
            auto numberHandle = numberInput.getHandle();

            slider.getSignal<float>("FloatValueChanged").connect([handle, numberHandle](float val) {
                if (!handle.is_alive() || !numberHandle.is_alive()) {
                    return;
                }
                core::ecs::entities::UINumberInput nbInput(core::ecs::EntityRegistry::GetEntityFromId(numberHandle));
                auto &nbInput_comp = nbInput.getComponentMutable<core::components::UINumberInput>();
                nbInput_comp.value = val;

                UIColorPicker colorPicker(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = colorPicker.getComponentMutable<core::components::UIColorPicker>();
                colorPicker.setColor({ comp.current_color.r, val, comp.current_color.b, comp.current_color.a });
            });
            numberInput.getSignal<float>("FloatValueChanged").connect([handle, sliderHandle](float val) {
                if (!handle.is_alive() || !sliderHandle.is_alive()) {
                    return;
                }
                UISlider slider(core::ecs::EntityRegistry::GetEntityFromId(sliderHandle));
                slider.setValue(val, false);

                UIColorPicker colorPicker(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = colorPicker.getComponentMutable<core::components::UIColorPicker>();
                colorPicker.setColor({ comp.current_color.r, val, comp.current_color.b, comp.current_color.a });
            });

            numberInput.getSignal<float>("FloatValueChanged").emit(1.0f);
        }
        { /* Blue slider */
            setupRow("B", std::string(RowB));

            auto color_panel = getChild(std::string(RowB) + "panel");

            auto slider = color_panel.getChild(std::string(RowB) + "slider");
            auto sliderHandle = slider.getHandle();

            auto numberInput = color_panel.getChild(std::string(RowB) + "input");
            auto numberHandle = numberInput.getHandle();

            slider.getSignal<float>("FloatValueChanged").connect([handle, numberHandle](float val) {
                if (!handle.is_alive() || !numberHandle.is_alive()) {
                    return;
                }
                core::ecs::entities::UINumberInput nbInput(core::ecs::EntityRegistry::GetEntityFromId(numberHandle));
                auto &nbInput_comp = nbInput.getComponentMutable<core::components::UINumberInput>();
                nbInput_comp.value = val;

                UIColorPicker colorPicker(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = colorPicker.getComponentMutable<core::components::UIColorPicker>();
                colorPicker.setColor({ comp.current_color.r, comp.current_color.g, val, comp.current_color.a });
            });
            numberInput.getSignal<float>("FloatValueChanged").connect([handle, sliderHandle](float val) {
                if (!handle.is_alive() || !sliderHandle.is_alive()) {
                    return;
                }
                UISlider slider(core::ecs::EntityRegistry::GetEntityFromId(sliderHandle));
                slider.setValue(val, false);

                UIColorPicker colorPicker(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = colorPicker.getComponentMutable<core::components::UIColorPicker>();
                colorPicker.setColor({ comp.current_color.r, comp.current_color.g, val, comp.current_color.a });
            });

            numberInput.getSignal<float>("FloatValueChanged").emit(1.0f);
        }
        { /* Alpha slider */
            setupRow("A", std::string(RowA));

            auto color_panel = getChild(std::string(RowA) + "panel");

            auto slider = color_panel.getChild(std::string(RowA) + "slider");
            auto sliderHandle = slider.getHandle();

            auto numberInput = color_panel.getChild(std::string(RowA) + "input");
            auto numberHandle = numberInput.getHandle();

            slider.getSignal<float>("FloatValueChanged").connect([handle, numberHandle](float val) {
                if (!handle.is_alive() || !numberHandle.is_alive()) {
                    return;
                }
                core::ecs::entities::UINumberInput nbInput(core::ecs::EntityRegistry::GetEntityFromId(numberHandle));
                auto &nbInput_comp = nbInput.getComponentMutable<core::components::UINumberInput>();
                nbInput_comp.value = val;

                UIColorPicker colorPicker(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = colorPicker.getComponentMutable<core::components::UIColorPicker>();
                colorPicker.setColor({ comp.current_color.r, comp.current_color.g, comp.current_color.b, val });
            });
            numberInput.getSignal<float>("FloatValueChanged").connect([handle, sliderHandle](float val) {
                if (!handle.is_alive() || !sliderHandle.is_alive()) {
                    return;
                }
                UISlider slider(core::ecs::EntityRegistry::GetEntityFromId(sliderHandle));
                slider.setValue(val, false);

                UIColorPicker colorPicker(core::ecs::EntityRegistry::GetEntityFromId(handle));
                auto &comp = colorPicker.getComponentMutable<core::components::UIColorPicker>();
                colorPicker.setColor({ comp.current_color.r, comp.current_color.g, comp.current_color.b, val });
            });

            numberInput.getSignal<float>("FloatValueChanged").emit(1.0f);
        }
    }

    void UIColorPicker::syncRow(std::string_view rowName, float value)
    {
        std::string panel_name = std::string(rowName) + "panel";
        Entity color_panel;
        for (auto &child : getChildren()) {
            if (child.name() == panel_name) {
                color_panel = child;
                break;
            }
        }
        if (!color_panel.getHandle().is_alive())
            return;

        auto slider_entity = color_panel.getChild(std::string(rowName) + "slider");
        auto number_input_entity = color_panel.getChild(std::string(rowName) + "input");

        UISlider(slider_entity.getHandle()).setValue(value, false);
        number_input_entity.getComponentMutable<core::components::UINumberInput>().value = value;
    }

    void UIColorPicker::setColor(const types::Color &color)
    {
        auto &comp = getComponentMutable<components::UIColorPicker>();
        comp.current_color = color;

        syncRow(RowR, color.r);
        syncRow(RowG, color.g);
        syncRow(RowB, color.b);
        syncRow(RowA, color.a);

        getSignal<types::Color>("ColorChanged").emit(comp.current_color);
    }

    types::Color UIColorPicker::getColor() const
    {
        return getComponent<components::UIColorPicker>().current_color;
    }

    Clay_ElementDeclaration UIColorPicker::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();
        return d;
    }

    void UIColorPicker::draw(ClaySdL3RendererData *data)
    {
        auto &comp = getComponentMutable<core::components::UIColorPicker>();
        getSignal<types::Color>("Update preview").emit(comp.current_color);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIColorPicker);
ATMO_REGISTER_COMPONENT(atmo::core::components::UIColorPicker)
