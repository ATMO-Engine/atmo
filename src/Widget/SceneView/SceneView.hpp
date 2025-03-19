#ifndef SceneView_HPP_
#define SceneView_HPP_

#include <flecs.h>
#include "../Widget.hpp"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"

class SceneView : public Widget
{
    public:
        SceneView(const flecs::world &ecs, SDL_Renderer *rend);
        ~SceneView() = default;

        void run() override;
        bool init();
    private:
        const flecs::world &_ecs;
        SDL_Renderer *_renderer = nullptr;
        int my_image_width = 0;
        int my_image_height = 0;
        SDL_Texture* my_texture = nullptr;
};

#endif /* !SceneView_HPP_ */
