#include "ui_number_input.hpp"
#include <charconv>
#include <cmath>
#include <format>
#include <optional>
#include <string>
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "ctre.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    namespace
    {
        using InputType = core::components::UIInput::InputType;

        double AsDouble(const std::variant<int, float> &value)
        {
            return std::visit([](auto v) { return static_cast<double>(v); }, value);
        }

        std::variant<int, float> Normalize(const std::variant<int, float> &value, InputType type)
        {
            if (type == InputType::Int)
                return static_cast<int>(std::lround(AsDouble(value)));
            return static_cast<float>(AsDouble(value));
        }

        std::string Trim(const std::string &str)
        {
            auto start = str.find_first_not_of(" \t");
            if (start == std::string::npos)
                return {};
            auto end = str.find_last_not_of(" \t");
            return str.substr(start, end - start + 1);
        }

        std::optional<std::variant<int, float>> Parse(const std::string &raw, InputType type)
        {
            std::string text = Trim(raw);
            if (!text.empty() && text.front() == '+')
                text.erase(0, 1);

            if (type == InputType::Int) {
                if (!ctre::match<"^-?[0-9]+$">(text))
                    return std::nullopt;
                int value = 0;
                auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
                if (ec != std::errc() || ptr != text.data() + text.size())
                    return std::nullopt;
                return value;
            }

            if (!ctre::match<"^-?([0-9]+(\\.[0-9]*)?|\\.[0-9]+)([eE][+\\-]?[0-9]+)?$">(text))
                return std::nullopt;
            try {
                float value = std::stof(text);
                if (!std::isfinite(value))
                    return std::nullopt;
                return value;
            } catch (...) {
                return std::nullopt;
            }
        }
    } // namespace

    void UINumberInput::RegisterSystems(flecs::world *world)
    {
        world->component<core::components::UINumberInput>();

        world->system<core::components::UINumberInput>("NumberInput_Update")
            .kind(flecs::OnStore)
            .each([](flecs::entity e, core::components::UINumberInput &comp) {
                if (comp.value == comp.prev_value)
                    return;

                UINumberInput input(e);
                if (input.isFocused())
                    return;

                comp.prev_value = comp.value;
                input.showValue();
            });
    }

    void UINumberInput::showValue()
    {
        const auto type = getComponent<core::components::UIInput>().input_type;
        const auto value = Normalize(getComponent<core::components::UINumberInput>().value, type);

        if (type == InputType::Int)
            setText(std::to_string(std::get<int>(value)));
        else
            setText(std::format("{:g}", std::get<float>(value)));
    }

    void UINumberInput::commit()
    {
        const auto type = getComponent<core::components::UIInput>().input_type;
        auto &comp = getComponentMutable<core::components::UINumberInput>();

        auto previous = Normalize(comp.value, type);
        auto next = Parse(getText(), type).value_or(previous);

        comp.value = next;
        comp.prev_value = next;
        showValue();

        if (next == previous)
            return;

        if (type == InputType::Int)
            getSignal<int>("IntValueChanged").emit(std::get<int>(next));
        else
            getSignal<float>("FloatValueChanged").emit(std::get<float>(next));
    }

    void UINumberInput::clear()
    {
        const auto type = getComponent<core::components::UIInput>().input_type;
        auto &comp = getComponentMutable<core::components::UINumberInput>();
        comp.value = Normalize(0, type);
        comp.prev_value = comp.value;
        showValue();
    }

    void UINumberInput::setNumber(int value)
    {
        auto &comp = getComponentMutable<core::components::UINumberInput>();
        comp.value = value;
        comp.prev_value = value;
        showValue();
    }

    void UINumberInput::setNumber(float value)
    {
        auto &comp = getComponentMutable<core::components::UINumberInput>();
        comp.value = value;
        comp.prev_value = value;
        showValue();
    }

    void UINumberInput::initialize()
    {
        UIInput::initialize();
        setComponent<core::components::UINumberInput>({});

        createSignal<float>("FloatValueChanged");
        createSignal<int>("IntValueChanged");

        getComponentMutable<core::components::UIInput>().input_type = InputType::Float;
        clear();
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UINumberInput);
