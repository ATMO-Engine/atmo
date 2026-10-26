#include "material_editor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <variant>

#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/ecs/entities/ui/ui_input/ui_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entities/ui/ui_slider/ui_slider.hpp"
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
        using core::types::Color;

        constexpr int sample_size = 64;
        constexpr float grid_cell = 10.0f;
        constexpr float sample_fraction = 0.6f;
        constexpr float panel_width = 340.0f;
        const Color text_color("#111827");
        const Color label_color("#374151");
        const Color muted_color("#6B7280");
        const Color error_color("#B91C1C");
        const Color field_color("#F3F4F6");
        const Color divider_color("#E5E7EB");
        const Color accent_color("#2563EB");

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

        std::shared_ptr<core::ecs::entities::UILabel> MakeLabel(const std::string &text, int size, Color color, bool bold = false)
        {
            auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            label->setText(text);
            label->setFontSize(size);
            label->setFontBold(bold);
            label->getComponentMutable<core::components::UI>().modulate = color;
            return label;
        }

        std::shared_ptr<core::ecs::entities::UI> MakeBox(core::components::Layout::Direction direction, std::uint16_t gap)
        {
            auto box = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            auto &layout = box->getComponentMutable<core::components::Layout>();
            layout.direction = direction;
            layout.width.type = SizingType::GROW;
            layout.child_gap = gap;
            layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
            return box;
        }

        std::shared_ptr<core::ecs::entities::UI> MakeField()
        {
            auto field = MakeBox(core::components::Layout::Direction::Vertical, 0);
            field->getComponentMutable<core::components::Layout>().clip.horizontal = true;
            return field;
        }

        void StyleInput(core::ecs::entities::Entity input)
        {
            input.getComponentMutable<core::components::UIInput>().background_color = field_color;
            input.getComponentMutable<core::components::Layout>().width.type = SizingType::GROW;
        }

        void StyleWidget(core::ecs::entities::Entity widget)
        {
            if (widget.getHandle().has<core::components::UIInput>()) {
                StyleInput(widget);
                return;
            }
            if (widget.getHandle().has<core::components::UIRect>())
                return;
            auto &layout = widget.getComponentMutable<core::components::Layout>();
            layout.width.type = SizingType::GROW;
            layout.child_gap = 6;
            for (auto &child : widget.getChildren())
                if (child.getHandle().has<core::components::UIInput>())
                    StyleInput(child);
        }

        std::string FileName(const std::string &path)
        {
            return std::filesystem::path(path).filename().string();
        }

        std::vector<std::byte> Pixels(int size, const std::function<Color(int, int)> &color)
        {
            std::vector<std::byte> pixels(static_cast<std::size_t>(size * size * 4));
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    const Color c = color(x, y);
                    std::byte *p = pixels.data() + ((static_cast<std::size_t>(y) * size) + x) * 4;
                    p[0] = std::byte{ static_cast<std::uint8_t>(c.r * 255.0f) };
                    p[1] = std::byte{ static_cast<std::uint8_t>(c.g * 255.0f) };
                    p[2] = std::byte{ static_cast<std::uint8_t>(c.b * 255.0f) };
                    p[3] = std::byte{ static_cast<std::uint8_t>(c.a * 255.0f) };
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
        m_window = container.getHandle().world().lookup("_Root");

        auto editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &editor_layout = editor_container->getComponentMutable<core::components::Layout>();
        editor_layout.width.type = SizingType::GROW;
        editor_layout.height.type = SizingType::GROW;
        editor_layout.padding = { 16, 16, 8, 16 };
        editor_layout.child_gap = 16;
        editor_container->setParent(container);

        auto panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &panel_layout = panel->getComponentMutable<core::components::Layout>();
        panel->getComponentMutable<core::components::UIRect>().color = Color::WHITE;
        panel->getComponentMutable<core::components::UIRect>().corner_radius = { 4, 4, 4, 4 };
        panel_layout.direction = core::components::Layout::Direction::Vertical;
        panel_layout.width.type = SizingType::FIXED;
        panel_layout.width.size = core::components::Layout::SizingAxis::MinMax{ panel_width, panel_width };
        panel_layout.height.type = SizingType::GROW;
        panel_layout.padding = { 16, 16, 16, 16 };
        panel_layout.child_gap = 8;
        panel->setParent(*editor_container);

        MakeLabel("atmo.editors.material_editor.shader", 14, text_color, true)->setParent(*panel);
        auto shader_field = MakeField();
        auto shader_input = core::ecs::EntityRegistry::Create<core::ecs::entities::UITextInput>("Entity::UI::UIInput::UITextInput");
        shader_input->getComponentMutable<core::components::UIInput>().placeholder = "project://shaders/effect.slang";
        StyleInput(*shader_input);
        shader_input->getSignal<std::string>("StringValueChanged").connect([this](std::string path) {
            if (!m_material)
                m_material = std::make_shared<core::resource::Material>();
            if (path == m_material->shaderPath())
                return;
            if (!m_material->setShader(path))
                setStatus(std::format("The shader {} could not be loaded, see the log", FileName(path)));
            else
                setStatus({});
            rebuildParams();
            updatePreview();
        });
        shader_input->setParent(*shader_field);
        shader_field->setParent(*panel);
        m_shader_input = shader_input->getHandle();

        auto info = MakeLabel("", 12, muted_color);
        info->setParent(*panel);
        m_info_label = info->getHandle();

        auto status_field = MakeField();
        auto status = MakeLabel("", 12, error_color);
        status->getComponentMutable<core::components::UI>().visible = false;
        status->setParent(*status_field);
        status_field->setParent(*panel);
        m_status_label = status->getHandle();

        auto divider = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        divider->getComponentMutable<core::components::UIRect>().color = divider_color;
        auto &divider_layout = divider->getComponentMutable<core::components::Layout>();
        divider_layout.width.type = SizingType::GROW;
        divider_layout.height.type = SizingType::FIXED;
        divider_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 1.0f, 1.0f };
        divider->setParent(*panel);

        MakeLabel("atmo.editors.material_editor.parameters", 14, text_color, true)->setParent(*panel);

        auto params = MakeBox(core::components::Layout::Direction::Vertical, 14);
        auto &params_layout = params->getComponentMutable<core::components::Layout>();
        params_layout.height.type = SizingType::GROW;
        params_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        params_layout.clip.vertical = true;
        params->setParent(*panel);
        m_params_container = params->getHandle();

        auto preview_area = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        preview_area->getComponentMutable<core::components::UIRect>().color = Color("#3D3D3D");
        auto &preview_layout = preview_area->getComponentMutable<core::components::Layout>();
        preview_layout.width.type = SizingType::GROW;
        preview_layout.height.type = SizingType::GROW;
        preview_layout.clip = { true, true };
        preview_area->setParent(*editor_container);

        createPreview();
        if (m_preview_viewport.isValid()) {
            auto preview = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
            preview->getComponentMutable<core::components::UIImage>().raw_texture =
                rendering::RenderingServer::Instance().viewportGetTexture(m_preview_viewport);
            preview->setParent(*preview_area);
            m_preview_image = preview->getHandle();
        }

        m_tick_callback = core::event::EventRegistry::SetCallBack<core::event::events::ProgressTickEvent>([this](core::event::events::ProgressTickEvent *) {
            if (m_material && m_material->shader() && (m_material->shader() != m_shown_shader || m_material->shader()->version() != m_shown_version))
                rebuildParams();
            syncParams();
            resizePreview();
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
            setStatus(std::format("Could not save {}", FileName(*p_file_path)));
    }

    void MaterialEditor::load()
    {
        if (!p_file_path)
            return;
        try {
            setMaterial(core::resource::ResourceManager::GetInstance().getResource<core::resource::Material>(*p_file_path));
            setStatus({});
        } catch (const std::exception &e) {
            spdlog::error("Material editor: {}", e.what());
            setStatus(std::format("Could not open {}, see the log", FileName(*p_file_path)));
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
        updateInfo();
        if (!m_shown_shader || !m_params_container.is_alive())
            return;

        core::ecs::entities::UI container(m_params_container);
        if (m_shown_shader->layout().params.empty())
            MakeLabel("atmo.editors.material_editor.no_parameters", 12, muted_color)->setParent(container);

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
                        slot->color = Color((*v)[0], (*v)[1], (*v)[2], (*v)[3]);
                    slot->field = SlotField<Color>(slot->name.c_str(), "color_picker");
                    slot->value = &slot->color;
                    break;
                case rendering::ShaderParamType::Texture:
                    if (const auto *v = std::get_if<core::resource::MaterialTextureRef>(&value))
                        slot->texture = v->path;
                    slot->field = SlotField<std::string>(slot->name.c_str(), "file_path");
                    slot->value = &slot->texture;
                    break;
            }
            addParamRow(container, param, *slot);
            m_slots.push_back(std::move(slot));
        }
    }

    void MaterialEditor::addParamRow(core::ecs::entities::UI &container, const rendering::ShaderParam &param, ParamSlot &slot)
    {
        auto row = MakeBox(core::components::Layout::Direction::Vertical, 4);
        row->getComponentMutable<core::components::Layout>().child_alignment.vertical = core::components::Layout::ChildAlignment::Start;

        auto header = MakeBox(core::components::Layout::Direction::Horizontal, 6);
        MakeLabel(param.name, 12, label_color)->setParent(*header);
        if (param.range)
            MakeLabel(std::format("{} – {}", param.range->at(0), param.range->at(1)), 11, muted_color)->setParent(*header);
        header->setParent(*row);

        const bool integer = slot.type == rendering::ShaderParamType::Int || slot.type == rendering::ShaderParamType::Uint;
        if (param.range && (integer || slot.type == rendering::ShaderParamType::Float)) {
            auto line = MakeBox(core::components::Layout::Direction::Horizontal, 10);

            auto slider = core::ecs::EntityRegistry::Create<core::ecs::entities::UISlider>("Entity::UI::UIRect::UISlider");
            slider->setType(
                integer ? core::components::UISlider::SliderType::Int : core::components::UISlider::SliderType::Float, param.range->at(0), param.range->at(1));
            slider->setValue(integer ? static_cast<float>(slot.integer) : slot.number, false);
            auto &slider_rect = slider->getComponentMutable<core::components::UIRect>();
            slider_rect.color = divider_color;
            slider_rect.border = {};
            slider_rect.corner_radius = { 3, 3, 3, 3 };
            auto &slider_layout = slider->getComponentMutable<core::components::Layout>();
            slider_layout.height.type = SizingType::FIXED;
            slider_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 14.0f, 14.0f };
            slider->getChild("Slider indicator").getComponentMutable<core::components::UIRect>().color = accent_color;
            slider->setParent(*line);

            auto input = core::ecs::EntityRegistry::Create<core::ecs::entities::UINumberInput>("Entity::UI::UIInput::UINumberInput");
            input->getComponentMutable<core::components::UIInput>().input_type =
                integer ? core::components::UIInput::InputType::Int : core::components::UIInput::InputType::Float;
            if (integer)
                input->setNumber(slot.integer);
            else
                input->setNumber(slot.number);
            input->getComponentMutable<core::components::UIInput>().background_color = field_color;
            auto &input_layout = input->getComponentMutable<core::components::Layout>();
            input_layout.width.type = SizingType::FIXED;
            input_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 72.0f, 72.0f };
            input->setParent(*line);

            const flecs::entity slider_handle = slider->getHandle();
            const flecs::entity input_handle = input->getHandle();
            if (integer) {
                slider->getSignal<int>("IntValueChanged").connect([&slot, input_handle](int value) {
                    slot.integer = value;
                    if (input_handle.is_alive())
                        core::ecs::entities::UINumberInput(input_handle).setNumber(value);
                });
                input->getSignal<int>("IntValueChanged").connect([&slot, slider_handle](int value) {
                    slot.integer = value;
                    if (slider_handle.is_alive())
                        core::ecs::entities::UISlider(slider_handle).setValue(static_cast<float>(value), false);
                });
            } else {
                slider->getSignal<float>("FloatValueChanged").connect([&slot, input_handle](float value) {
                    slot.number = value;
                    if (input_handle.is_alive())
                        core::ecs::entities::UINumberInput(input_handle).setNumber(value);
                });
                input->getSignal<float>("FloatValueChanged").connect([&slot, slider_handle](float value) {
                    slot.number = value;
                    if (slider_handle.is_alive())
                        core::ecs::entities::UISlider(slider_handle).setValue(value, false);
                });
            }
            line->setParent(*row);
        } else {
            auto field = MakeField();
            if (auto widget = meta::WidgetRegistry::Instance().create(*field, slot.value, slot.field))
                StyleWidget(*widget);
            else
                spdlog::warn("The material editor has no widget for parameter {}", param.name);
            field->setParent(*row);
        }
        row->setParent(container);
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

    void MaterialEditor::createPreview()
    {
        auto &server = rendering::RenderingServer::Instance();
        if (!server.isInitialized())
            return;

        m_preview_viewport = server.viewportCreate();
        server.viewportSetSize(m_preview_viewport, { 1, 1 });
        server.viewportSetClearColor(m_preview_viewport, Color("#3D3D3D"));
        m_preview_canvas = server.canvasCreate();
        server.viewportSetCanvas(m_preview_viewport, m_preview_canvas);

        const Color grid_light("#F3F4F6");
        const Color grid_dark("#D1D5DB");
        m_preview_grid = server.texture2dCreate(
            { .size = { 2, 2 }, .debug_name = "material preview grid" }, Pixels(2, [&](int x, int y) { return (x + y) % 2 == 0 ? grid_light : grid_dark; }));

        const Color warm("#E8B45A");
        const Color cool("#4A6EC8");
        m_preview_sample = server.texture2dCreate(
            { .size = { sample_size, sample_size }, .debug_name = "material preview sample" },
            Pixels(sample_size, [&](int x, int y) { return ((x / 8) + (y / 8)) % 2 == 0 ? warm : cool; }));

        m_preview_background = server.canvasItemCreate(m_preview_canvas);
        m_preview_item = server.canvasItemCreate(m_preview_canvas);
    }

    void MaterialEditor::resizePreview()
    {
        if (!m_preview_viewport.isValid() || !m_preview_image.is_alive())
            return;
        const core::types::Vector2 size = m_preview_image.get<core::components::UIImage>().rendered_size;
        core::types::Vector2 dpi{ 1.0f, 1.0f };
        if (m_window.is_alive() && m_window.has<core::components::Window>())
            dpi = m_window.get<core::components::Window>().dpi_scale;
        if (size.x < 1.0f || size.y < 1.0f)
            return;
        if (size.x == m_preview_size.x && size.y == m_preview_size.y && dpi.x == m_preview_dpi.x && dpi.y == m_preview_dpi.y)
            return;
        m_preview_size = size;
        m_preview_dpi = dpi;

        auto &server = rendering::RenderingServer::Instance();
        server.viewportSetSize(
            m_preview_viewport, { std::max(1, static_cast<int>(std::lround(size.x * dpi.x))), std::max(1, static_cast<int>(std::lround(size.y * dpi.y))) });
        server.viewportSetCanvasTransform(m_preview_viewport, rendering::Affine2d::Scaling(dpi));

        server.canvasItemClear(m_preview_background);
        server.canvasItemAddTextureRect(
            m_preview_background,
            { { 0.0f, 0.0f }, { size.x, size.y } },
            m_preview_grid,
            { .source = rendering::Rect2{ { 0.0f, 0.0f }, { size.x / grid_cell, size.y / grid_cell } },
              .filter = rendering::TextureFilter::Nearest,
              .repeat = true });

        const float side = std::floor(std::min(size.x, size.y) * sample_fraction);
        server.canvasItemClear(m_preview_item);
        server.canvasItemAddTextureRect(
            m_preview_item,
            { { std::floor((size.x - side) * 0.5f), std::floor((size.y - side) * 0.5f) }, { side, side } },
            m_preview_sample,
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
        server.textureFree(m_preview_grid);
        server.textureFree(m_preview_sample);
        m_preview_viewport = {};
    }

    void MaterialEditor::setStatus(const std::string &text)
    {
        if (!m_status_label.is_alive())
            return;
        core::ecs::entities::UILabel label(m_status_label);
        label.setText(text);
        label.getComponentMutable<core::components::UI>().visible = !text.empty();
    }

    void MaterialEditor::updateInfo()
    {
        if (!m_info_label.is_alive())
            return;
        std::string_view key = "atmo.editors.material_editor.no_shader";
        if (m_material && m_material->shader())
            key = m_material->kind() == rendering::ShaderKind::Viewport ? "atmo.editors.material_editor.viewport_material"
                                                                        : "atmo.editors.material_editor.canvas_material";
        core::ecs::entities::UILabel(m_info_label).setText(std::string(key));
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::MaterialEditor);
