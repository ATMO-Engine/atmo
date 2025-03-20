#include "SceneView.hpp"
#include <SDL3/SDL.h>
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "spdlog/spdlog.h"
#include <SDL3/SDL.h>
#include <iostream>

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool LoadTextureFromMemory(const void *data, size_t data_size, SDL_Renderer *renderer, SDL_Texture **out_texture,
                           int *out_width, int *out_height)
{
    int image_width = 0;
    int image_height = 0;
    int channels = 4;
    unsigned char *image_data =
        stbi_load_from_memory((const unsigned char *)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == nullptr) {
        fprintf(stderr, "Failed to load image: %s\n", stbi_failure_reason());
        return false;
    }

    SDL_Surface *surface = SDL_CreateSurfaceFrom(image_width, image_height, SDL_PIXELFORMAT_RGBA32, (void *)image_data,
                                                 channels * image_width);
    if (surface == nullptr) {
        fprintf(stderr, "Failed to create SDL surface: %s\n", SDL_GetError());
        return false;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr)
        fprintf(stderr, "Failed to create SDL texture: %s\n", SDL_GetError());

    *out_texture = texture;
    *out_width = image_width;
    *out_height = image_height;

    SDL_DestroySurface(surface);
    stbi_image_free(image_data);

    return true;
}

bool LoadTextureFromFile(const char *file_name, SDL_Renderer *renderer, SDL_Texture **out_texture, int *out_width,
                         int *out_height)
{
    FILE *f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void *file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = LoadTextureFromMemory(file_data, file_size, renderer, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

SceneView::SceneView(const flecs::world &ecs, SDL_Renderer *rend) : _ecs(ecs), _renderer(rend), _query(ecs.query_builder<Node>().cached().build()), _images()
{
}

SceneView::~SceneView()
{
    for (auto& entry : _images) {
        SDL_Texture* texture = entry.second.first;
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
    _images.clear();
}

void SceneView::run()
{
    ImGui::Text("Scene View");
    displayImages();
}

bool SceneView::init()
{
    return true;
}

bool isDisplayable(flecs::entity entity)
{
    bool has_transform = entity.has<Transform>();
    bool has_image = entity.has<Image>();

    return has_transform && has_image;
}

void SceneView::drawImage(flecs::entity entity)
{
    auto transformC = entity.get_mut<Transform>();
    auto imageC = entity.get_mut<Image>();

    auto it = _images.find(imageC->imagePath);
    if (it == _images.end())
        return;

    ImVec2 scaled_dimensions(it->second.second.x * transformC->scale[0], it->second.second.y * transformC->scale[1]);

    float x = transformC->position[0];
    float y = transformC->position[1];
    ImVec2 image_position(x, y);
    ImGui::SetCursorPos(image_position);
    ImGui::Image((ImTextureID)(intptr_t)it->second.first, scaled_dimensions);
}

void SceneView::loadImageTexture(flecs::entity entity)
{
    auto imageC = entity.get_mut<Image>();


    if (!imageC || imageC->imagePath.empty())
        return;

    auto it = _images.find(imageC->imagePath);

    if (it == _images.end()) {
        std::string path = "./assets/" + imageC->imagePath;
        int my_image_width = 0;
        int my_image_height = 0;
        SDL_Texture* my_texture = nullptr;

        bool ret = LoadTextureFromFile(path.c_str(), _renderer, &my_texture, &my_image_width, &my_image_height);
        IM_ASSERT(ret);

        _images.insert({imageC->imagePath, {my_texture, ImVec2((float)my_image_width, (float)my_image_height)}});
    }
}


void SceneView::displayImages()
{
    _query.each(
        [this](flecs::entity e, Node &node) {
        loadImageTexture(e);
        if (isDisplayable(e)) {
            drawImage(e);
        }
    });
}
