#ifndef SceneView_HPP_
#define SceneView_HPP_

#include <flecs.h>
#include "../Widget.hpp"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"

class SceneView : public Widget
{
    public:
        SceneView(const flecs::world &ecs);
        ~SceneView() = default;

        void run() override;
        void init(SDL_Window *window);
    private:
        const flecs::world &_ecs;
        SDL_Renderer *_renderer;
        int my_image_width, my_image_height;
        SDL_Texture* my_texture;
};

#endif /* !SceneView_HPP_ */
