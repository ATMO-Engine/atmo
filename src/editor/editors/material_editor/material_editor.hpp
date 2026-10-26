#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/resource/material.hpp"
#include "core/types.hpp"
#include "editor/editor_registry.hpp"
#include "editor/editors/editor.hpp"
#include "flecs.h"
#include "meta/type_info.hpp"
#include "server/rendering/rid.hpp"

namespace atmo::editor
{
    class MaterialEditor : public EditorRegistry::Registrable<MaterialEditor, Editor>
    {
    public:
        using EditorRegistry::Registrable<MaterialEditor, Editor>::Registrable;

        ~MaterialEditor() override;

        static constexpr std::string_view LocalName()
        {
            return "MaterialEditor";
        }

        std::string_view name() override
        {
            return "atmo.editors.material_editor.name";
        }

        std::string_view description() override
        {
            return "atmo.editors.material_editor.description";
        }

        std::string_view iconPath() override
        {
            return "project://assets/icons/dot.svg";
        }

        std::string_view getTypeName() const override
        {
            return FullName();
        }

        void init(atmo::core::ecs::entities::UI &container) override;
        void createTools() override;

        void save() override;
        void load() override;

    private:
        struct ParamSlot {
            std::string name;
            server::rendering::ShaderParamType type = server::rendering::ShaderParamType::Float;
            float number = 0.0f;
            int integer = 0;
            core::types::Vector2 vector2;
            core::types::Vector3 vector3;
            core::types::Vector4 vector4;
            core::types::Color color{ core::types::Color::WHITE };
            std::string texture;
            meta::FieldInfo field;
            void *value = nullptr;
        };

        void setMaterial(std::shared_ptr<core::resource::Material> material);
        void rebuildParams();
        void syncParams();
        void updatePreview();
        void createPreview(core::types::Vector2 dpi_scale);
        void destroyPreview() noexcept;
        void setStatus(const std::string &text);

        std::shared_ptr<core::resource::Material> m_material;
        std::shared_ptr<core::resource::Shader> m_shown_shader;
        std::uint64_t m_shown_version = 0;
        std::vector<std::unique_ptr<ParamSlot>> m_slots;

        flecs::entity m_root_handle;
        flecs::entity m_params_container;
        flecs::entity m_shader_input;
        flecs::entity m_status_label;
        std::uint32_t m_tick_callback = 0;
        bool m_has_tick_callback = false;

        server::rendering::ViewportRid m_preview_viewport;
        server::rendering::CanvasRid m_preview_canvas;
        server::rendering::CanvasItemRid m_preview_background;
        server::rendering::CanvasItemRid m_preview_item;
        server::rendering::TextureRid m_preview_checker;
        server::rendering::MaterialRid m_preview_material;
        bool m_preview_post = false;
    };
} // namespace atmo::editor
