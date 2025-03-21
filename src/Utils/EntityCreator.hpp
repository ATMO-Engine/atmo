#include <array>
#include <flecs.h>
#include <string>

#pragma once

struct Node
{
        std::string description = "";
};

struct Transform
{
        std::array<float, 2> position = {0.0f, 0.0f};
        std::array<float, 2> rotation = {0.0f, 0.0f};
        std::array<float, 2> scale = {1.0f, 1.0f};
        float skew = 0.0f;
};

struct Image
{
        std::string imagePath = "";
};

class EntityCreator
{
    public:
        EntityCreator() = delete;
        ~EntityCreator() = delete;

        static void registerComponents(flecs::world &ecs)
        {
            ecs.component<Node>();
            ecs.component<Transform>();
            ecs.component<Image>();
        }

        static flecs::entity createEntity(flecs::world &ecs, const std::string &name)
        {
            return ecs.entity(name.c_str()).set<Node>({});
        }
};
