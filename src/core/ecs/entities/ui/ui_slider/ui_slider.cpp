#include "ui_slider.hpp"
#include <algorithm>
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/input/input_manager.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void UISlider::RegisterSystems(flecs::world *world) {}

    void UISlider::Unregister(flecs::world *) {}

    void UISlider::initialize()
    {
        UIRect::initialize();

        setComponent<components::UISlider>({});

        createSignal<float>("InternalFloatValueChanged");
        createSignal<float>("ValueFloatChanged");
        createSignal<int>("InternalIntValueChanged");
        createSignal<int>("ValueIntChanged");

        auto &layout = getComponentMutable<core::components::Layout>();
        layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;

        auto &rect = getComponentMutable<core::components::UIRect>();
        rect.color = core::types::Color::WHITE;
        rect.border.color = core::types::Color::BLACK;
        rect.border.left = 1;
        rect.border.right = 1;
        rect.border.top = 1;
        rect.border.bottom = 1;

        auto indicator = core::ecs::EntityRegistry::Create("Entity::UI::UIRect");
        auto &indicator_rect = indicator->getComponentMutable<core::components::UIRect>();
        auto &indicator_layout = indicator->getComponentMutable<core::components::Layout>();

        indicator_rect.color = core::types::Color::BLACK;

        indicator_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        indicator_layout.height.size = 1.0f;
        indicator_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        indicator_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 4.0f, 4.0f };

        indicator->rename("Slider indicator");
        indicator->setParent(*this);
    }

    void UISlider::setType(components::UISlider::SliderType type, float min, float max)
    {
        auto &comp = getComponentMutable<components::UISlider>();
        comp.type = type;

        if (type == components::UISlider::SliderType::Int) {
            comp.min = static_cast<int>(min);
            comp.max = static_cast<int>(max);
            comp.value = static_cast<int>(min);
        } else {
            comp.min = min;
            comp.max = max;
            comp.value = min;
        }
    }

    void UISlider::setValue(float value)
    {
        auto &comp = getComponentMutable<components::UISlider>();

        float fmin = std::visit([](auto v) { return static_cast<float>(v); }, comp.min);
        float fmax = std::visit([](auto v) { return static_cast<float>(v); }, comp.max);
        float clamped = std::clamp(value, fmin, fmax);

        if (comp.type == components::UISlider::SliderType::Int) {
            comp.value = static_cast<int>(clamped);
            getSignal<int>("InternalIntValueChanged").emit(clamped);
        } else {
            comp.value = clamped;
            getSignal<float>("InternalFloatValueChanged").emit(clamped);
        }
    }

    float UISlider::getValue() const
    {
        return std::visit([](auto v) { return static_cast<float>(v); }, getComponent<components::UISlider>().value);
    }

    Clay_ElementDeclaration UISlider::buildDecl()
    {
        Clay_ElementDeclaration d = UIRect::buildDecl();
        d.id = CLAY_IDI("UISlider", static_cast<uint32_t>(getID()));
        return d;
    }

    void UISlider::draw(ClaySdL3RendererData *data)
    {
        auto &comp = getComponentMutable<components::UISlider>();

        Clay_ElementData elementData = Clay_GetElementData(CLAY_IDI("UISlider", static_cast<uint32_t>(getID())));
        if (elementData.found)
            comp.bounds = elementData.boundingBox;

        auto mousePos = core::InputManager::GetMousePosition();
        bool isHovered = Clay_Hovered();

        if (isHovered && core::InputManager::IsJustPressed("ui_click"))
            comp.dragging = true;
        if (!core::InputManager::IsPressed("ui_click"))
            comp.dragging = false;

        if (comp.dragging && comp.bounds.width > 0.0f) {
            float relative = (mousePos.x - comp.bounds.x) / comp.bounds.width;
            float fmin = std::visit([](auto v) { return static_cast<float>(v); }, comp.min);
            float fmax = std::visit([](auto v) { return static_cast<float>(v); }, comp.max);
            float newValue = fmin + std::clamp(relative, 0.0f, 1.0f) * (fmax - fmin);
            setValue(newValue);
        }

        // Positionne l'indicateur via padding left
        if (comp.bounds.width > 0.0f) {
            float fmin = std::visit([](auto v) { return static_cast<float>(v); }, comp.min);
            float fmax = std::visit([](auto v) { return static_cast<float>(v); }, comp.max);
            float fval = std::visit([](auto v) { return static_cast<float>(v); }, comp.value);

            float percent = (fval - fmin) / (fmax - fmin);
            float offsetX = percent * (comp.bounds.width - 4.0f);

            auto &layout = getComponentMutable<core::components::Layout>();
            layout.padding.left = static_cast<uint16_t>(offsetX);
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UISlider);
ATMO_REGISTER_COMPONENT(atmo::core::components::UISlider)
