#ifndef SceneHierarchy_HPP_
#define SceneHierarchy_HPP_

#include <flecs.h>
#include "../Widget.hpp"

class SceneHierarchy : public Widget {
    public:
        SceneHierarchy(flecs::world &world);
        ~SceneHierarchy() = default;

        void run() override;

    protected:
        const std::string widgetName = "Scene Hierarchy";

    private:
        flecs::world &world;
};

#endif /* !SceneHierarchy_HPP_ */
