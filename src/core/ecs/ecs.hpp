#pragma once

#include <exception>
#include <flecs.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "components.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/scene/scene_manager.hpp"

namespace atmo::core::ecs
{
    class ECS
    {
    private:
        flecs::world m_world;
        scene::SceneManager m_scene_manager;

    public:
        ECS() = default;
        ~ECS() = default;
        void stop();
        void reset();

        std::shared_ptr<entities::Scene> createScene(const std::string &scene_name, bool singleton);
        void changeScene(std::shared_ptr<entities::Scene> scene);
        void changeSceneToFile(std::string_view scene_path);

        bool progress(float delta_time = 0.0f) const
        {
            return m_world.progress(delta_time);
        }
    };
} // namespace atmo::core::ecs
