#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <vector>

#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/subresource.hpp"
#include "glaze/glaze.hpp"

/**
 * @brief Teaches Glaze how to (de)serialize a vector of polymorphic SubResource pointers, e.g.
 * `std::vector<std::shared_ptr<Shape2d>>`. Each element is written as `{"type": <registry name>, "data": <fields>}`
 * so the concrete type can be reconstructed via `SubResourceRegistry::Create` on read.
 *
 * Must be visible wherever a component holding such a vector is registered (i.e. wherever
 * `glz::write_json`/`glz::read_json` is instantiated for that component), since Glaze resolves this specialization
 * at the point of template instantiation.
 */
namespace glz
{
    template <class T>
        requires std::derived_from<T, atmo::core::resource::SubResource>
    struct to<JSON, std::vector<std::shared_ptr<T>>> {
        template <auto Opts> static void op(auto &&value, is_context auto &&ctx, auto &&...args)
        {
            std::vector<glz::generic> arr;
            arr.reserve(value.size());

            for (auto &el : value) {
                glz::generic obj;
                if (el) {
                    obj["type"] = std::string(el->getTypeName());
                    obj["data"] = glz::read_json<glz::generic>(el->serialize()).value_or(glz::generic{});
                }
                arr.push_back(std::move(obj));
            }

            serialize<JSON>::template op<Opts>(arr, ctx, args...);
        }
    };

    template <class T>
        requires std::derived_from<T, atmo::core::resource::SubResource>
    struct from<JSON, std::vector<std::shared_ptr<T>>> {
        template <auto Opts> static void op(auto &&value, is_context auto &&ctx, auto &&it, auto end)
        {
            std::vector<glz::generic> arr;
            parse<JSON>::template op<Opts>(arr, ctx, it, end);
            if (bool(ctx.error)) [[unlikely]] {
                return;
            }

            value.clear();
            value.reserve(arr.size());

            for (auto &entry : arr) {
                if (!entry.is_object() || !entry.contains("type"))
                    continue;

                auto shape = atmo::core::resource::SubResourceRegistry::Create<T>(entry["type"].template as<std::string>());
                if (!shape)
                    continue;

                if (entry.contains("data")) {
                    auto data_str = entry["data"].dump().value_or("{}");
                    shape->deserialize(data_str);
                }

                value.push_back(std::move(shape));
            }
        }
    };
} // namespace glz
