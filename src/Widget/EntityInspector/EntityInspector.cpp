#include "EntityInspector.hpp"
#include "flecs/addons/cpp/c_types.hpp"
#include "flecs/addons/cpp/type.hpp"

EntityInspector::EntityInspector(const flecs::world &ecs, flecs::entity_t &selectedEntity) :
    ecs(ecs), selectedEntity(selectedEntity)
{
}

static void TextCentered(const std::string &text) {
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth   = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text("%s", text.c_str());
}

void EntityInspector::run() {
    if (selectedEntity == -1)
        return;

    auto entity = ecs.entity(selectedEntity);
    if (!entity.is_valid() || !entity.is_alive())
        return;

    if (entity.name().length() < 1)
        TextCentered("Unnamed Entity");
    else
        TextCentered(entity.name().c_str());
    ImGui::Separator();
    // show all components of entity
    entity.each([&](flecs::id id) {
        // check if component is flecs::Identifier
        if (entity.world().has<EcsPrivate>(id) || e.world().has<EcsHidden>(id))
            ImGui::Text("Identifier");
        else
            ImGui::Text("%s", id.type_id().str().c_str());
    });
}
