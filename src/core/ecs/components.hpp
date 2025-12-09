#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

#include <box2d/box2d.h>
#include <flecs.h>
#include <spdlog/spdlog.h>
#include "SDL3/SDL_rect.h"
#include "clay.h"

#include "core/resource/handle.hpp"
#include "core/types.hpp"
#include "luau/luau.hpp"


#define BEGIN_REFLECT(Type)                     \
    namespace                                   \
    {                                           \
        void register_##Type(flecs::world &ecs) \
        {                                       \
            using Self = Type;                  \
            auto c = ecs.component<Self>();

#define FIELD(m) c.member<decltype(Self::m)>(#m);

#define END_REFLECT(Type)                                                                 \
    }                                                                                     \
    const bool Type##_ref_reg = (component_registry().push_back(&register_##Type), true); \
    }

namespace atmo
{
    namespace core
    {
        class ComponentManager
        {
        public:
            virtual ~ComponentManager() = default;

            struct Managed {
                ComponentManager *ptr;
            };

            flecs::entity entity;
        };

        namespace components
        {
            using RegistrationFn = void (*)(flecs::world &);

            static inline std::vector<RegistrationFn> &component_registry()
            {
                static std::vector<RegistrationFn> registry;
                return registry;
            }

            struct Scene {
                std::string scene_name;
                bool singleton{ false };
                b2WorldId world_id{ b2_nullWorldId };
            };

            struct Transform2D {
                types::vector2 position{ 0.0f, 0.0f };
                types::vector2 g_position{ 0.0f, 0.0f };

                float rotation{ 0.0f };
                float g_rotation{ 0.0f };

                types::vector2 scale{ 1.0f, 1.0f };
                types::vector2 g_scale{ 1.0f, 1.0f };
            };
            BEGIN_REFLECT(Transform2D)
            FIELD(position)
            FIELD(rotation)
            FIELD(scale)
            END_REFLECT(Transform2D)

            struct PhysicsBody2D {
                b2BodyId body_id{ b2_nullBodyId };
            };

            struct Window {
                std::string title;
                types::vector2i size;
            };
            BEGIN_REFLECT(Window)
            FIELD(title)
            FIELD(size)
            END_REFLECT(Window)

            struct Script {
                std::string file;
            };
            BEGIN_REFLECT(Script)
            FIELD(file)
            END_REFLECT(Script)

            struct Sprite2D {
                std::string texture_path;
                resource::Handle m_handle;
                SDL_FRect m_dest_rect{ 0, 0, 0, 0 };
                types::vector2 texture_size{ 0.0f, 0.0f };
            };
            BEGIN_REFLECT(Sprite2D)
            FIELD(texture_path)
            END_REFLECT(Sprite2D)

            namespace UI
            {
                // Clay_ElementDeclaration decl;
                // Clay_TextElementConfig textConfig;
                struct UI {
                    bool visible{ true };
                    types::rgba modulate{ 1.0f, 1.0f, 1.0f, 1.0f };
                    types::rgba self_modulate{ 1.0f, 1.0f, 1.0f, 1.0f };
                };
                BEGIN_REFLECT(UI)
                FIELD(visible)
                FIELD(modulate)
                FIELD(self_modulate)
                END_REFLECT(UI)

                struct Text {
                    std::string content;
                    // Clay_TextElementConfig config;
                };
                BEGIN_REFLECT(Text)
                FIELD(content)
                // FIELD(config)
                END_REFLECT(Text)
            } // namespace UI

            static void register_core_components(flecs::world ecs)
            {
                flecs::opaque<std::string>(ecs)
                    .as_type(ecs_id(ecs_string_t))
                    .serialize([](const ecs_serializer_t *s, const std::string *v) { return s->value(*v); })
                    .assign_string([](std::string *dst, const char *src) { *dst = src ? src : ""; })
                    .assign_null([](std::string *dst) { *dst = {}; })
                    .clear([](std::string *dst) { dst->clear(); });

                types::register_core_types(ecs);

                for (auto fn : component_registry()) {
                    fn(ecs);
                }
            }
        } // namespace components
    } // namespace core
} // namespace atmo
