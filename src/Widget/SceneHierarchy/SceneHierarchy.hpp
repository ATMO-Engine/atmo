#ifndef SceneHierarchy_HPP_
#define SceneHierarchy_HPP_

#include <flecs.h>
#include "../Widget.hpp"

struct Engine {
    bool process;
};

class SceneHierarchy : public Widget {
    public:
        SceneHierarchy(const flecs::world &ecs);
        ~SceneHierarchy() = default;

        void run() override;

    protected:
        const std::string widgetName = "Scene Hierarchy";

    private:
        const flecs::world &ecs;
        const flecs::query<Engine> query;
};

#endif /* !SceneHierarchy_HPP_ */
