#pragma once

#include <string>
#include <vector>

#include <flecs.h>

#include "core/types.hpp"
#include "meta/meta_registry.hpp"

namespace atmo::core::components
{
    template <typename Elem, typename Vector = std::vector<Elem>> flecs::opaque<Vector, Elem> static std_vector_support(flecs::world &world)
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

        atmo::meta::MetaRegistry::Instance().registerAllFlecs(world);
    }
} // namespace atmo::core::components
