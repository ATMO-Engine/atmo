#include "material_editor.hpp"

#include <array>
#include <cstddef>
#include <format>
#include <variant>

#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/progress_tick_event/progress_tick_event.hpp"
#include "core/resource/resource_manager.hpp"
#include "meta/widget_registry.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

namespace atmo::editor
{
    namespace
    {
        namespace rendering = server::rendering;
        using SizingType = core::components::Layout::SizingAxis::SizingAxisType;

        constexpr float preview_size = 256.0f;
        constexpr int checker_size = 32;

        template <typename T> void SlotGet(const void *slot, void *out)
        {
            *static_cast<T *>(out) = *static_cast<const T *>(slot);
        }

        template <typename T> void SlotSet(void *slot, const void *value)
        {
            *static_cast<T *>(slot) = *static_cast<const T *>(value);
        }

        template <typename T> meta::FieldInfo SlotField(const char *name, const char *widget)
        {
            meta::FieldInfo field;
            field.name = name;
            field.widget = widget;
            field.size = sizeof(T);
            field.get = &SlotGet<T>;
            field.set = &SlotSet<T>;
            return field;
        }

        std::shared_ptr<core::ecs::entities::UILabel> MakeLabel(const std::string &text, int size)
        {
            auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            label->setText(text);
            label->setFontSize(size);
            label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            return label;
        }

        std::vector<std::byte> Checker()
        {
            std::vector<std::byte> pixels(static_cast<std::size_t>(checker_size * checker_size * 4));
            for (int y = 0; y < checker_size; ++y) {
                for (int x = 0; x < checker_size; ++x) {
                    const bool light = ((x / 8) + (y / 8)) % 2 == 0;
                    std::byte *p = pixels.data() + ((static_cast<std::size_t>(y) * checker_size) + x) * 4;
                    p[0] = std::byte{ static_cast<std::uint8_t>(light ? 0xE8 : 0x4A) };
                    p[1] = std::byte{ static_cast<std::uint8_t>(light ? 0xB4 : 0x6E) };
                    p[2] = std::byte{ static_cast<std::uint8_t>(light ? 0x5A : 0xC8) };
                    p[3] = std::byte{ 0xFF };
                }
            }
            return pixels;
        }
    } // namespace

    MaterialEditor::~MaterialEditor()
    {
        if (m_has_tick_callback)
            core::event::EventRegistry::RemoveCallBack<core::event::events::ProgressTickEvent>(m_tick_callback);
        destroyPreview();
    }

    void MaterialEditor::init(atmo::core::ecs::entities::UI &container)
    {
        m_root_handle = container.getHandle();

        flecs::entity root = container.getHandle().world().lookup("_Root");
        core::types::Vector2 dpi_scale{ 1.0f, 1.0f };
        if (root.is_valid() && root.has<core::components::Window>())
            dpi_scale = root.get<core::components::Window>().dpi_scale;

        auto editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &editor_layout = editor_container->getComponentMutable<core::components::Layout>();
        editor_layout.width.type = SizingType::GROW;
        editor_layout.height.type = SizingType::GROW;
        editor_layout.padding = { 16, 16, 8, 16 };
        editor_layout.child_gap = 16;
        editor_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        editor_container->setParent(container);

        auto panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &panel_layout = panel->getComponentMutable<core::components::Layout>();
        panel->getComponentMutable<core::components::UIRect>().color = core::types::Color::WHITE;
        panel->getComponentMutable<core::components::UIRect>().corner_radius = { 4, 4, 4, 4 };
        panel_layout.direction = core::components::Layout::Direction::Vertical;
        panel_layout.width.type = SizingType::PERCENT;
        panel_layout.width.size = 0.35f;
        panel_layout.height.type = SizingType::GROW;
        panel_layout.padding = { 12, 12, 12, 12 };
        panel_layout.child_gap = 8;
        panel_layout.clip.horizontal = true;
        panel->setParent(*editor_container);

        MakeLabel("atmo.editors.material_editor.shader", 12)->setParent(*panel);
        auto shader_input = core::ecs::EntityRegistry::Create<core::ecs::entities::UITextInput>("Entity::UI::UIInput::UITextInput");
        shader_input->getComponentMutable<core::components::UIInput>().placeholder = "project://shaders/effect.slang";
        shader_input->getComponentMutable<core::components::Layout>().width.type = SizingType::GROW;
        shader_input->getSignal<std::string>("StringValueChanged").connect([this](std::string path) {
            if (!m_material)
                m_material = std::make_shared<core::resource::Material>();
            if (path == m_material->shaderPath())
                return;
            if (!m_material->setShader(path))
                setStatus(std::format("The shader {} could not be loaded, see the log", path));
            else
                setStatus({});
            rebuildParams();
            updatePreview();
        });
        shader_input->setParent(*panel);
        m_shader_input = shader_input->getHandle();

        auto status = MakeLabel("", 12);
        status->getComponentMutable<core::components::UI>().modulate = core::types::Color("#B91C1C");
        status->setParent(*panel);
        m_status_label = status->getHandle();

        auto params = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &params_layout = params->getComponentMutable<core::components::Layout>();
        params_layout.direction = core::components::Layout::Direction::Vertical;
        params_layout.width.type = SizingType::GROW;
        params_layout.height.type = SizingType::FIT;
        params_layout.child_gap = 8;
        params->setParent(*panel);
        m_params_container = params->getHandle();

        createPreview(dpi_scale);
        if (m_preview_viewport.isValid()) {
            auto frame = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            auto &frame_layout = frame->getComponentMutable<core::components::Layout>();
            frame_layout.width.type = SizingType::FIXED;
            frame_layout.width.size = core::components::Layout::SizingAxis::MinMax{ preview_size, preview_size };
            frame_layout.height.type = SizingType::FIXED;
            frame_layout.height.size = core::components::Layout::SizingAxis::MinMax{ preview_size, preview_size };
            frame->setParent(*editor_container);

            auto preview = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
            preview->getComponentMutable<core::components::UIImage>().raw_texture =
                rendering::RenderingServer::Instance().viewportGetTexture(m_preview_viewport);
            preview->setParent(*frame);
        }

        m_tick_callback = core::event::EventRegistry::SetCallBack<core::event::events::ProgressTickEvent>([this](core::event::events::ProgressTickEvent *) {
            if (m_material && m_material->shader() && (m_material->shader() != m_shown_shader || m_material->shader()->version() != m_shown_version))
                rebuildParams();
            syncParams();
            updatePreview();
        });
        m_has_tick_callback = true;

        setMaterial(std::make_shared<core::resource::Material>());
    }

    void MaterialEditor::createTools() {}

    void MaterialEditor::save()
    {
        if (!p_file_path || !m_material)
            return;
        syncParams();
        if (m_material->save(*p_file_path))
            setStatus({});
        else
            setStatus(std::format("Could not save {}", *p_file_path));
    }

    void MaterialEditor::load()
    {
        if (!p_file_path)
            return;
        try {
            setMaterial(core::resource::ResourceManager::GetInstance().getResource<core::resource::Material>(*p_file_path));
            setStatus({});
        } catch (const std::exception &e) {
            setStatus(e.what());
        }
    }

    void MaterialEditor::setMaterial(std::shared_ptr<core::resource::Material> material)
    {
        m_material = std::move(material);
        if (m_shader_input.is_alive())
            core::ecs::entities::UITextInput(m_shader_input).setValue(m_material ? m_material->shaderPath() : std::string());
        rebuildParams();
        updatePreview();
    }

    void MaterialEditor::rebuildParams()
    {
        if (m_params_container.is_alive()) {
            core::ecs::entities::UI container(m_params_container);
            for (auto &child : container.getChildren()) child.destroy();
        }
        m_slots.clear();
        m_shown_shader = m_material ? m_material->shader() : nullptr;
        m_shown_version = m_shown_shader ? m_shown_shader->version() : 0;
        if (!m_shown_shader || !m_params_container.is_alive())
            return;

        core::ecs::entities::UI container(m_params_container);
        for (const rendering::ShaderParam &param : m_shown_shader->layout().params) {
            auto slot = std::make_unique<ParamSlot>();
            slot->name = param.name;
            slot->type = param.type;

            core::resource::MaterialValue value = m_material->get(param.name).value_or(core::resource::Material::DefaultValue(param));
            const double *number = std::get_if<double>(&value);
            switch (param.type) {
                case rendering::ShaderParamType::Int:
                case rendering::ShaderParamType::Uint:
                    slot->integer = number ? static_cast<int>(*number) : 0;
                    slot->field = SlotField<int>(slot->name.c_str(), "int");
                    slot->value = &slot->integer;
                    break;
                case rendering::ShaderParamType::Float:
                    slot->number = number ? static_cast<float>(*number) : 0.0f;
                    slot->field = SlotField<float>(slot->name.c_str(), "float");
                    slot->value = &slot->number;
                    break;
                case rendering::ShaderParamType::Float2:
                    if (const auto *v = std::get_if<std::array<float, 2>>(&value))
                        slot->vector2 = { (*v)[0], (*v)[1] };
                    slot->field = SlotField<core::types::Vector2>(slot->name.c_str(), "vector2");
                    slot->value = &slot->vector2;
                    break;
                case rendering::ShaderParamType::Float3:
                    if (const auto *v = std::get_if<std::array<float, 3>>(&value))
                        slot->vector3 = { (*v)[0], (*v)[1], (*v)[2] };
                    slot->field = SlotField<core::types::Vector3>(slot->name.c_str(), "vector3");
                    slot->value = &slot->vector3;
                    break;
                case rendering::ShaderParamType::Float4:
                    if (const auto *v = std::get_if<std::array<float, 4>>(&value))
                        slot->vector4 = { (*v)[0], (*v)[1], (*v)[2], (*v)[3] };
                    slot->field = SlotField<core::types::Vector4>(slot->name.c_str(), "vector4");
                    slot->value = &slot->vector4;
                    break;
                case rendering::ShaderParamType::Color:
                    if (const auto *v = std::get_if<std::array<float, 4>>(&value))
                        slot->color = core::types::Color((*v)[0], (*v)[1], (*v)[2], (*v)[3]);
                    slot->field = SlotField<core::types::Color>(slot->name.c_str(), "color_picker");
                    slot->value = &slot->color;
                    break;
                case rendering::ShaderParamType::Texture:
                    if (const auto *v = std::get_if<core::resource::MaterialTextureRef>(&value))
                        slot->texture = v->path;
                    slot->field = SlotField<std::string>(slot->name.c_str(), "file_path");
                    slot->value = &slot->texture;
                    break;
            }
            if (param.range) {
                slot->field.has_range = true;
                slot->field.range_min = param.range->at(0);
                slot->field.range_max = param.range->at(1);
            }

            auto row = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            row->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Vertical;
            const std::string label = param.range ? std::format("{} ({} to {})", param.name, param.range->at(0), param.range->at(1)) : param.name;
            MakeLabel(label, 12)->setParent(*row);
            if (!meta::WidgetRegistry::Instance().create(*row, slot->value, slot->field))
                spdlog::warn("The material editor has no widget for parameter {}", param.name);
            row->setParent(container);
            m_slots.push_back(std::move(slot));
        }
    }

    void MaterialEditor::syncParams()
    {
        if (!m_material || !m_material->shader())
            return;
        for (const auto &slot : m_slots) {
            core::resource::MaterialValue value;
            switch (slot->type) {
                case rendering::ShaderParamType::Int:
                case rendering::ShaderParamType::Uint:
                    value = static_cast<double>(slot->integer);
                    break;
                case rendering::ShaderParamType::Float:
                    value = static_cast<double>(slot->number);
                    break;
                case rendering::ShaderParamType::Float2:
                    value = std::array{ slot->vector2.x, slot->vector2.y };
                    break;
                case rendering::ShaderParamType::Float3:
                    value = std::array{ slot->vector3.x, slot->vector3.y, slot->vector3.z };
                    break;
                case rendering::ShaderParamType::Float4:
                    value = std::array{ slot->vector4.x, slot->vector4.y, slot->vector4.z, slot->vector4.w };
                    break;
                case rendering::ShaderParamType::Color:
                    value = std::array{ slot->color.r, slot->color.g, slot->color.b, slot->color.a };
                    break;
                case rendering::ShaderParamType::Texture:
                    {
                        core::resource::MaterialTextureRef texture;
                        if (auto current = m_material->get(slot->name))
                            if (const auto *ref = std::get_if<core::resource::MaterialTextureRef>(&*current))
                                texture = *ref;
                        texture.path = slot->texture;
                        value = texture;
                        break;
                    }
            }
            if (m_material->get(slot->name) != value)
                m_material->set(slot->name, std::move(value));
        }
    }

    void MaterialEditor::createPreview(core::types::Vector2 dpi_scale)
    {
        auto &server = rendering::RenderingServer::Instance();
        if (!server.isInitialized())
            return;

        m_preview_viewport = server.viewportCreate();
        server.viewportSetSize(m_preview_viewport, { static_cast<int>(preview_size * dpi_scale.x), static_cast<int>(preview_size * dpi_scale.y) });
        server.viewportSetClearColor(m_preview_viewport, core::types::Color("#1F2937"));
        server.viewportSetCanvasTransform(m_preview_viewport, rendering::Affine2d::Scaling(dpi_scale));
        m_preview_canvas = server.canvasCreate();
        server.viewportSetCanvas(m_preview_viewport, m_preview_canvas);

        m_preview_checker = server.texture2dCreate({ .size = { checker_size, checker_size }, .debug_name = "material preview" }, Checker());
        m_preview_background = server.canvasItemCreate(m_preview_canvas);
        server.canvasItemAddRect(m_preview_background, { { 0.0f, 0.0f }, { preview_size, preview_size * 0.5f } }, core::types::Color("#F9FAFB"));
        server.canvasItemAddRect(m_preview_background, { { 0.0f, preview_size * 0.5f }, { preview_size, preview_size * 0.5f } }, core::types::Color("#111827"));

        m_preview_item = server.canvasItemCreate(m_preview_canvas);
        server.canvasItemAddTextureRect(
            m_preview_item,
            { { preview_size * 0.125f, preview_size * 0.125f }, { preview_size * 0.75f, preview_size * 0.75f } },
            m_preview_checker,
            { .filter = rendering::TextureFilter::Nearest });
    }

    void MaterialEditor::updatePreview()
    {
        if (!m_preview_viewport.isValid())
            return;
        auto &server = rendering::RenderingServer::Instance();
        const rendering::MaterialRid material = m_material ? m_material->rid() : rendering::MaterialRid{};
        const bool post = m_material && m_material->kind() == rendering::ShaderKind::Viewport;
        if (material == m_preview_material && post == m_preview_post)
            return;

        m_preview_material = material;
        m_preview_post = post;
        server.canvasItemSetMaterial(m_preview_item, post ? rendering::MaterialRid{} : material);
        const std::array chain{ material };
        server.viewportSetPostProcess(
            m_preview_viewport, post && material.isValid() ? std::span<const rendering::MaterialRid>(chain) : std::span<const rendering::MaterialRid>());
    }

    void MaterialEditor::destroyPreview() noexcept
    {
        auto &server = rendering::RenderingServer::Instance();
        if (!server.isInitialized())
            return;
        server.canvasItemFree(m_preview_item);
        server.canvasItemFree(m_preview_background);
        server.canvasFree(m_preview_canvas);
        server.viewportFree(m_preview_viewport);
        server.textureFree(m_preview_checker);
        m_preview_viewport = {};
    }

    void MaterialEditor::setStatus(const std::string &text)
    {
        if (m_status_label.is_alive())
            core::ecs::entities::UILabel(m_status_label).setText(text);
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::MaterialEditor);
