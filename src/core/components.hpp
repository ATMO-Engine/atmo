#pragma once

#include <cstdint>
#include <cstdlib>
#include <flecs.h>
#include <spdlog/spdlog.h>
#include <string>

#include "core/types.hpp"
#include "luau/luau.hpp"

namespace atmo
{
    namespace core
    {
        namespace components
        {
            typedef struct Window {
                bool main{false};
                std::string title;
                types::vector2i size;
            } Window;

            typedef struct Script {
                std::string file;
            } Script;

            struct UITransform {
                float x{0.0f};
                float y{0.0f};
                float width{0.0f};
                float height{0.0f};
                float rotation{0.0f};
                bool visible{true};
            };

            struct UIColor {
                float r{1.0f};
                float g{1.0f};
                float b{1.0f};
                float a{1.0f};
            };

            struct UIText {
                std::string content;
                std::string font;
                float font_size{16.f};
                UIColor color{1.0f, 1.0f, 1.0f, 1.0f};
                bool wrap{false};
            };

            struct UIImage {
                std::string texture;
                bool nine_slice{false};
                float slice_left{0.0f}, slice_right{0.0f}, slice_top{0.0f}, slice_bottom{0.0f};
            };

            struct UIRect {
                UIColor color{1.0f, 1.0f, 1.0f, 1.0f};
                types::vector4 corner_radius{0.0f, 0.0f, 0.0f, 0.0f};
                types::vector4i border_thickness{0, 0, 0, 0};
                UIColor border_color{0.0f, 0.0f, 0.0f, 1.0f};
            };

            enum class UILayoutDirection {
                Horizontal,
                Vertical
            };

            struct UIStack {
                UILayoutDirection direction{UILayoutDirection::Vertical};
                types::vector4i spacing{0, 0, 0, 0};
                types::vector4i padding{0, 0, 0, 0};
                bool expand{false};
                std::uint16_t childGap{0};
            };

            struct UIScroll {
                bool horizontal{true};
                bool vertical{true};
                float scroll_x{0.0f};
                float scroll_y{0.0f};
            };
        } // namespace components

        class ComponentManager
        {
        public:
            virtual ~ComponentManager() = default;

            template <typename Component>
            static void registerSystems(
                flecs::world ecs, std::unordered_map<flecs::entity_t, ComponentManager *> &component_managers
            )
            {
            }

            flecs::entity entity;
        };
    } // namespace core
} // namespace atmo
