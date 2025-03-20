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
<<<<<<< Updated upstream
=======
        void displayImages();
        void drawImage(flecs::entity entity);
        void loadImageTexture(flecs::entity entity);
>>>>>>> Stashed changes

    private:
        const flecs::world &_ecs;
        SDL_Renderer *_renderer = nullptr;
<<<<<<< Updated upstream
        int my_image_width = 0;
        int my_image_height = 0;
        SDL_Texture *my_texture = nullptr;
=======
        const flecs::query<Node> _query;
        std::map<std::string, std::pair<SDL_Texture *, ImVec2>> _images;
>>>>>>> Stashed changes
};

#endif /* !SceneView_HPP_ */
