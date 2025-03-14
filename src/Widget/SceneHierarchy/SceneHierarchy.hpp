#ifndef SceneHierarchy_HPP_
#define SceneHierarchy_HPP_

#include <flecs.h>
#include "../../Utils/SafeQueue.hpp"
#include "../Widget.hpp"

struct Engine
{
        bool process;
};

class SceneHierarchy : public Widget
{
    public:
        SceneHierarchy(const flecs::world &ecs, flecs::entity_t &selectedEntity);
        ~SceneHierarchy() = default;

        void run() override;
        void logEntity(flecs::entity e, unsigned int depth);

    protected:
        const std::string widgetName = "Scene Hierarchy";

    private:
        const flecs::world &ecs;
        const flecs::query<Engine> query;
        SafeQueue<flecs::entity_t> deleteQueue;
        flecs::entity_t &selectedEntity;
};

#endif /* !SceneHierarchy_HPP_ */
