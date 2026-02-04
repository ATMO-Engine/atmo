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

#include "SDL3/SDL_render.h"

#define BEGIN_REFLECT(Type)                     \
    namespace                                   \
    {                                           \
        void register_##Type(flecs::world &ecs) \
        {                                       \
            using Self = Type;                  \
            auto c = ecs.component<Self>();

#define FIELD(m) c.member<decltype(Self::m)>(#m);
#define NAMED_FIELD(name, m) c.member<decltype(Self::m)>(name);

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
                bool singleton{ false };
                b2WorldId world_id{ b2_nullWorldId };
            };
            BEGIN_REFLECT(Scene)
            FIELD(singleton)
            END_REFLECT(Scene)

            struct Transform2D {
                types::Vector2 position{ 0.0f, 0.0f };
                types::Vector2 g_position{ 0.0f, 0.0f };

                float rotation{ 0.0f };
                float g_rotation{ 0.0f };

                types::Vector2 scale{ 1.0f, 1.0f };
                types::Vector2 g_scale{ 1.0f, 1.0f };
            };
            BEGIN_REFLECT(Transform2D)
            FIELD(position)
            FIELD(rotation)
            FIELD(scale)
            END_REFLECT(Transform2D)

            struct PhysicsBody2D {
                b2BodyId body_id{ b2_nullBodyId };
                b2BodyDef body_def{ b2DefaultBodyDef() };
                types::Shape2DType shape{ types::Shape2DType::None };
            };
            BEGIN_REFLECT(PhysicsBody2D)
            NAMED_FIELD("Shape", shape)
            END_REFLECT(PhysicsBody2D)

            struct RectangleShape2d {
                types::Vector2 size{ 1.0f, 1.0f };
            };
            BEGIN_REFLECT(RectangleShape2d)
            FIELD(size)
            END_REFLECT(RectangleShape2d)

            struct CircleShape2d {
                float radius{ 0.5f };
            };
            BEGIN_REFLECT(CircleShape2d)
            FIELD(radius)
            END_REFLECT(CircleShape2d)

            struct CapsuleShape2d {
                float radius{ 0.5f };
                float height{ 1.0f };
            };
            BEGIN_REFLECT(CapsuleShape2d)
            FIELD(radius)
            FIELD(height)
            END_REFLECT(CapsuleShape2d)

            struct PolygonShape2d {
                std::vector<types::Vector2> points;
            };
            BEGIN_REFLECT(PolygonShape2d)
            FIELD(points)
            END_REFLECT(PolygonShape2d)

            struct StaticBody2D {
                types::Vector2 position{ 0.0f, 0.0f };
                float rotation{ 0.0f };
            };
            BEGIN_REFLECT(StaticBody2D)
            FIELD(position)
            FIELD(rotation)
            END_REFLECT(StaticBody2D)

            struct DynamicBody2D {
            };

            struct KinematicBody2D {
            };

            struct Window {
                std::string title;
                types::Vector2i size;
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
                resource::Handle<SDL_Surface> m_handle;
                SDL_FRect m_dest_rect{ 0, 0, 0, 0 };
                types::Vector2 texture_size{ 0.0f, 0.0f };
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
                    types::ColorRGBA modulate{ 1.0f, 1.0f, 1.0f, 1.0f };
                    types::ColorRGBA self_modulate{ 1.0f, 1.0f, 1.0f, 1.0f };
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

            template <typename Elem, typename Vector = std::vector<Elem>> flecs::opaque<Vector, Elem> std_vector_support(flecs::world &world)
            {
                return flecs::opaque<Vector, Elem>()
                    .as_type(world.vector<Elem>())

                    .serialize([](const flecs::serializer *s, const Vector *data) {
                        for (const auto &el : *data) {
                            s->value(el);
                        }
                        return 0;
                    })

                    .count([](const Vector *data) { return data->size(); })

                    .resize([](Vector *data, size_t size) { data->resize(size); })

                    .ensure_element([](Vector *data, size_t elem) {
                        if (data->size() <= elem) {
                            data->resize(elem + 1);
                        }

                        return &data->data()[elem];
                    });
            }

            static void register_core_components(flecs::world world)
            {
                types::register_core_types(world);

                world.component<std::string>()
                    .opaque(flecs::String)
                    .serialize([](const flecs::serializer *s, const std::string *data) {
                        const char *str = data->c_str();
                        return s->value(flecs::String, &str);
                    })
                    .assign_string([](std::string *data, const char *value) { *data = value; });

                world.component<std::vector<types::Vector2>>().opaque(std_vector_support<types::Vector2>);

                for (auto fn : component_registry()) {
                    fn(world);
                }
            }
        } // namespace components
    } // namespace core
} // namespace atmo
