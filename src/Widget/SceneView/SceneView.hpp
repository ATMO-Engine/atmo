#ifndef SceneView_HPP_
#define SceneView_HPP_

#include "../Widget.hpp"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "../../Utils/EntityCreator.hpp"
#include "imgui.h"
#include <iostream>
#include <map>

class SceneView : public Widget
{
    public:
        SceneView(const flecs::world &ecs, SDL_Renderer *rend);
        ~SceneView();

        void run() override;
        bool init();
        void displayImages();
        void drawImage(flecs::entity entity);
        void loadImageTexture(flecs::entity entity);

    private:
        const flecs::world &_ecs;
        SDL_Renderer *_renderer = nullptr;
        const flecs::query<Node> _query;
        std::map<std::string, std::pair<SDL_Texture *, ImVec2>> _images;
};

#endif /* !SceneView_HPP_ */
