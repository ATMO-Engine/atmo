#ifndef SceneHierarchy_HPP_
#define SceneHierarchy_HPP_

#include <flecs.h>
#include "../../Utils/EntityCreator.hpp"
#include "../../Utils/SafeQueue.hpp"
#include "../Widget.hpp"
#include "imgui.h"

class SceneHierarchy : public Widget
{
    public:
        SceneHierarchy(flecs::world &ecs, flecs::entity_t &selectedEntity);
        ~SceneHierarchy() = default;

        void run() override;
        void logEntity(flecs::entity e, unsigned int depth);

    protected:
        const std::string widgetName = "Scene Hierarchy";

    private:
        flecs::world &ecs;
        const flecs::query<Node> query;
        SafeQueue<flecs::entity_t> deleteQueue;
        flecs::entity_t &selectedEntity;
};

#endif /* !SceneHierarchy_HPP_ */
