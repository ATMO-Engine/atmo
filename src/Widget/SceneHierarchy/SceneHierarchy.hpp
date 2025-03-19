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

        std::string *getRenameBuffer() { return &renameBuffer; }

    protected:
        const std::string widgetName = "Scene Hierarchy";
        void renameEntity();
        void createEntity();

    private:
        flecs::world &ecs;
        const flecs::query<Node> query;
        SafeQueue<flecs::entity_t> deleteQueue;
        flecs::entity_t &selectedEntity;
        flecs::entity_t rightClickedEntity;
        std::string renameBuffer;
        bool openRenamePopup = false;
        bool openCreateEntityPopup = false;
        flecs::entity_t newEntityParent = -1;
};

#endif /* !SceneHierarchy_HPP_ */
