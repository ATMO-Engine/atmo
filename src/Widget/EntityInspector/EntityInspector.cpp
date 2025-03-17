#include "EntityInspector.hpp"
#include "flecs.h"
#include "flecs/addons/cpp/c_types.hpp"
#include "flecs/addons/cpp/type.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "spdlog/spdlog.h"

EntityInspector::EntityInspector(const flecs::world &ecs, flecs::entity_t &selectedEntity) :
    _ecs(ecs), _selectedEntity(selectedEntity), _identifier(ecs.component<flecs::Identifier>())
{
}

static void TextCentered(const std::string &text)
{
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text("%s", text.c_str());
}

void EntityInspector::drawFieldString(const std::string &label, std::string *value)
{
    ImGui::Text("%s", label.c_str());
    ImGui::PushItemWidth(-1);
    ImGui::InputText(std::format("##%s", label).c_str(), value, ImGuiInputTextFlags_ElideLeft);
    ImGui::PopItemWidth();
}

void EntityInspector::drawFieldVector2(const std::string &label, std::array<float, 2> *value)
{
    ImGui::Text("%s", label.c_str());
    ImGui::PushItemWidth(-1);
    ImGui::InputFloat2(std::format("##%s", label).c_str(), value->data(), "%.2f");
    ImGui::PopItemWidth();
}

void EntityInspector::drawFieldFloat(const std::string &label, float *value)
{
    ImGui::Text("%s", label.c_str());
    ImGui::PushItemWidth(-1);
    ImGui::InputFloat(std::format("##%s", label).c_str(), value, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
}

void EntityInspector::drawComponent(flecs::entity entity, flecs::id componentId)
{
    auto component = _ecs.component(componentId);

    ImGui::BeginChild(componentId.type_id().str().c_str(), ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
        ImGui::Text("%s", componentId.str().c_str());

        ImGui::Separator();

        if (componentId == _ecs.component<Node>()) {
            auto st = entity.get_mut<Node>();
            drawData(st, entity);
        } else if (componentId == _ecs.component<Transform>()) {
            auto st = entity.get_mut<Transform>();
            drawData(st, entity);
        } else {
            spdlog::warn("Component {} not implemented", componentId.str().c_str());
        }
    ImGui::EndChild();
}

void EntityInspector::drawData(Node *node, flecs::entity entity)
{
    drawFieldString("Description", &node->description);
}

void EntityInspector::drawData(Transform *transform, flecs::entity entity)
{
    const std::map<std::string, std::array<float, 2> *> fields = {
        {"Position", &transform->position},
        {"Rotation", &transform->rotation},
        {"Scale", &transform->scale}
    };

    int id = 0;
    for (const auto &[label, value] : fields) {
        ImGui::PushID(id++);
        drawFieldVector2(label, value);
        ImGui::PopID();
    }
    drawFieldFloat("Skew", &transform->skew);
}

void EntityInspector::run() {
    if (_selectedEntity == -1)
        return;

    auto entity = _ecs.entity(_selectedEntity);
    if (!entity.is_valid() || !entity.is_alive())
        return;

    if (entity.name().length() < 1)
        TextCentered("Unnamed Entity");
    else
        TextCentered(entity.name().c_str());
    ImGui::Separator();

    entity.each([&](flecs::id id) {
        auto c = _ecs.component(id);
        if (id.is_pair() && id.first() == flecs::ChildOf)
            return;
        if (id.is_pair() && id.type_id() && id.type_id().str() == _identifier.type_id().str())
            return;
        drawComponent(entity, id);
    });
}
