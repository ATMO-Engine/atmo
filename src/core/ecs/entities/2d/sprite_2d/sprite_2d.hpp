#pragma once

#include <string>
#include "SDL3/SDL_surface.h"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/handle.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct Sprite2d {
        std::string texture_path;
        resource::Handle<SDL_Surface> m_handle;
        types::Vector2 texture_size{ 0.0f, 0.0f };
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Sprite2d> {
    static constexpr const char *name = "Sprite2d";
    static constexpr const char *category = "2D";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::Sprite2d::texture_path>("texture_path").withWidget("file_path"));
};

namespace atmo::core::ecs::entities
{
    class Sprite2d : public EntityRegistry::Registrable<Sprite2d, Entity2d>
    {
    public:
        using EntityRegistry::Registrable<Sprite2d, Entity2d>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Sprite2d";
        }

        void setTexturePath(const std::string &path);
        std::string_view getTexturePath() const noexcept;

        types::Vector2 getTextureSize() const noexcept;
    };
} // namespace atmo::core::ecs::entities
