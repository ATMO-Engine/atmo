#ifndef EntityInspector_HPP_
#define EntityInspector_HPP_

#include <flecs.h>
#include "../SceneHierarchy/SceneHierarchy.hpp"
#include "imgui.h"

class EntityInspector : public Widget
{
    public:
        EntityInspector(const flecs::world &ecs, flecs::entity_t &selectedEntity);
        ~EntityInspector() = default;

        void run() override;

        void drawData(Node *node, flecs::entity entity);
        void drawData(Transform *transform, flecs::entity entity);
        void drawComponent(flecs::entity entity, flecs::id component);
        void drawAddComponentsButtons(flecs::entity entity);

    protected:
        const std::string widgetName = "Entity Inspector";

    private:
        const flecs::world &_ecs;
        flecs::entity_t &_selectedEntity;
        flecs::component<flecs::Identifier> _identifier;

        void drawFieldString(const std::string &label, std::string *value);
        void drawFieldVector2(const std::string &label, std::array<float, 2> *value);
        void drawFieldFloat(const std::string &label, float *value);
};

#endif /* !EntityInspector_HPP_ */
