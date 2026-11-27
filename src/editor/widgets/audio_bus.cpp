#include <string>
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/widget_registry.hpp"
#include "server/audio/bus.hpp"

namespace
{
    std::string nextBus(const std::string &current)
    {
        const auto bus = atmo::server::audio::ParseBus(current);
        const std::size_t next = bus ? atmo::server::audio::BusIndex(*bus) + 1 : 0;
        return std::string(atmo::server::audio::bus_names[next % atmo::server::audio::bus_names.size()]);
    }
} // namespace

std::optional<atmo::core::ecs::entities::Entity> createAudioBusWidget(atmo::core::ecs::entities::Entity parent, void *value, const atmo::meta::FieldInfo &field)
{
    auto button = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
    auto label = atmo::core::ecs::EntityRegistry::Create<atmo::core::ecs::entities::UILabel>("Entity::UI::UILabel");

    std::string current;
    if (value)
        field.get(value, &current);
    label->setText(current);
    label->setFontSize(14);
    label->getComponentMutable<atmo::core::components::UI>().modulate = atmo::core::types::Color::BLACK;
    label->setParent(*button);

    button->getSignal<>("Pressed").connect([value, field, handle = label->getHandle()]() {
        if (!value || !field.set)
            return;

        std::string bus;
        field.get(value, &bus);
        bus = nextBus(bus);
        field.set(value, &bus);
        if (handle.is_alive())
            atmo::core::ecs::entities::UILabel(handle).setText(bus);
    });

    return *button;
}

void updateAudioBusWidget(atmo::core::ecs::entities::Entity widget, void *value, const atmo::meta::FieldInfo &field)
{
    std::string current;
    field.get(value, &current);
    atmo::core::ecs::entities::UILabel(widget.getChildren()[0].getHandle()).setText(current);
}

ATMO_REGISTER_WIDGET("audio_bus", createAudioBusWidget, {}, updateAudioBusWidget);
