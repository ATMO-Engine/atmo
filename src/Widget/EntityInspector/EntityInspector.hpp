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

    protected:
        const std::string widgetName = "Entity Inspector";

    private:
        const flecs::world &ecs;
        flecs::entity_t &selectedEntity;
};

#endif /* !EntityInspector_HPP_ */
