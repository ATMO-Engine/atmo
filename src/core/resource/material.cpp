#include "material.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>

#include "core/resource/resource_manager.hpp"
#include "glaze/glaze.hpp"
#include "project/file_system.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::resource
{
    namespace
    {
        namespace rendering = server::rendering;

        template <std::size_t N> std::optional<std::array<float, N>> Array(const MaterialValue &value)
        {
            if (const auto *array = std::get_if<std::array<float, N>>(&value))
                return *array;
            return std::nullopt;
        }

        std::optional<MaterialValue> ParseValue(const glz::generic &json)
        {
            if (json.is_number())
                return json.get_number();
            if (json.is_boolean())
                return json.get<bool>() ? 1.0 : 0.0;
            if (json.is_array()) {
                const auto &array = json.get_array();
                std::vector<float> numbers;
                for (const glz::generic &element : array) {
                    if (!element.is_number())
                        return std::nullopt;
                    numbers.push_back(static_cast<float>(element.get_number()));
                }
                switch (numbers.size()) {
                    case 2:
                        return std::array{ numbers[0], numbers[1] };
                    case 3:
                        return std::array{ numbers[0], numbers[1], numbers[2] };
                    case 4:
                        return std::array{ numbers[0], numbers[1], numbers[2], numbers[3] };
                    default:
                        return std::nullopt;
                }
            }
            if (json.is_object()) {
                const auto &object = json.get_object();
                MaterialTextureRef texture;
                if (auto it = object.find("texture"); it != object.end() && it->second.is_string())
                    texture.path = it->second.get_string();
                if (auto it = object.find("filter"); it != object.end() && it->second.is_string())
                    texture.filter = it->second.get_string() == "nearest" ? rendering::TextureFilter::Nearest : rendering::TextureFilter::Linear;
                if (auto it = object.find("repeat"); it != object.end() && it->second.is_boolean())
                    texture.repeat = it->second.get<bool>();
                return texture;
            }
            return std::nullopt;
        }

        glz::generic WriteValue(const MaterialValue &value)
        {
            return std::visit(
                [](const auto &v) -> glz::generic {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, double>) {
                        return v;
                    } else if constexpr (std::is_same_v<T, MaterialTextureRef>) {
                        glz::generic object;
                        object["texture"] = v.path;
                        object["filter"] = v.filter == rendering::TextureFilter::Nearest ? "nearest" : "linear";
                        object["repeat"] = v.repeat;
                        return object;
                    } else {
                        glz::generic::array_t array;
                        for (float component : v) array.emplace_back(static_cast<double>(component));
                        return array;
                    }
                },
                value);
        }
    } // namespace

    Material::~Material()
    {
        rendering::RenderingServer::Instance().materialFree(m_rid);
    }

    std::shared_ptr<Material> Material::FromJson(std::string_view json, std::string &error)
    {
        glz::generic root;
        if (auto parse_error = glz::read_json(root, json)) {
            error = glz::format_error(parse_error, json);
            return nullptr;
        }
        if (!root.is_object()) {
            error = "a material file must contain a JSON object";
            return nullptr;
        }

        auto material = std::make_shared<Material>();
        const auto &object = root.get_object();
        if (auto params = object.find("params"); params != object.end() && params->second.is_object()) {
            for (const auto &[name, json_value] : params->second.get_object()) {
                if (std::optional<MaterialValue> value = ParseValue(json_value))
                    material->m_values.emplace_back(name, std::move(*value));
                else
                    spdlog::warn("Material parameter '{}' has an unsupported value", name);
            }
        }
        if (auto shader = object.find("shader"); shader != object.end() && shader->second.is_string())
            material->setShader(shader->second.get_string());
        return material;
    }

    std::string Material::toJson() const
    {
        glz::generic root;
        root["shader"] = m_shader_path;
        root["params"] = glz::generic::object_t{};
        for (const auto &[name, value] : m_values) root["params"][name] = WriteValue(value);

        std::string out;
        if (glz::write_json(root, out))
            return "{}";
        return glz::prettify_json(out);
    }

    bool Material::save(const std::string &path) const
    {
        std::ofstream out(project::FileSystem::ResolvePath(path), std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            spdlog::error("Failed to save material {}", path);
            return false;
        }
        out << toJson();
        return out.good();
    }

    bool Material::setShader(const std::string &shader_path)
    {
        auto &server = rendering::RenderingServer::Instance();
        server.materialFree(m_rid);
        m_rid = {};
        m_shader = nullptr;
        m_shader_path = shader_path;
        if (shader_path.empty())
            return true;

        try {
            m_shader = ResourceManager::GetInstance().getResource<Shader>(shader_path);
        } catch (const std::exception &e) {
            spdlog::error("Material shader {} could not be loaded: {}", shader_path, e.what());
            return false;
        }
        if (!m_shader)
            return false;

        m_rid = server.materialCreate(m_shader->rid());
        m_shader_version = m_shader->version();
        applyAll();
        return true;
    }

    const std::string &Material::shaderPath() const noexcept
    {
        return m_shader_path;
    }

    const std::shared_ptr<Shader> &Material::shader() const noexcept
    {
        return m_shader;
    }

    rendering::MaterialRid Material::rid()
    {
        if (m_shader && m_shader->version() != m_shader_version) {
            m_shader_version = m_shader->version();
            applyAll();
        }
        return m_rid;
    }

    std::optional<rendering::ShaderKind> Material::kind() const noexcept
    {
        if (!m_shader)
            return std::nullopt;
        return m_shader->kind();
    }

    bool Material::set(std::string_view name, MaterialValue value)
    {
        if (m_shader && !m_shader->layout().find(name)) {
            spdlog::warn("Material: shader {} has no parameter '{}'", m_shader_path, name);
            return false;
        }

        const std::string key(name);
        if (m_shader && !apply(key, value))
            return false;

        auto it = std::ranges::find(m_values, name, &std::pair<std::string, MaterialValue>::first);
        if (it != m_values.end())
            it->second = std::move(value);
        else
            m_values.emplace_back(key, std::move(value));
        return true;
    }

    std::optional<MaterialValue> Material::get(std::string_view name) const
    {
        auto it = std::ranges::find(m_values, name, &std::pair<std::string, MaterialValue>::first);
        if (it == m_values.end())
            return std::nullopt;
        return it->second;
    }

    const std::vector<std::pair<std::string, MaterialValue>> &Material::values() const noexcept
    {
        return m_values;
    }

    MaterialValue Material::DefaultValue(const rendering::ShaderParam &param)
    {
        switch (param.type) {
            case rendering::ShaderParamType::Int:
            case rendering::ShaderParamType::Uint:
            case rendering::ShaderParamType::Float:
                return param.range ? static_cast<double>(param.range->at(0)) : 0.0;
            case rendering::ShaderParamType::Float2:
                return std::array<float, 2>{};
            case rendering::ShaderParamType::Float3:
                return std::array<float, 3>{};
            case rendering::ShaderParamType::Float4:
                return std::array<float, 4>{};
            case rendering::ShaderParamType::Color:
                return std::array{ 1.0f, 1.0f, 1.0f, 1.0f };
            case rendering::ShaderParamType::Texture:
                return MaterialTextureRef{};
        }
        return 0.0;
    }

    bool Material::apply(const std::string &name, const MaterialValue &value)
    {
        if (!m_shader || !m_rid.isValid())
            return false;
        const rendering::ShaderParam *param = m_shader->layout().find(name);
        if (!param)
            return false;

        std::optional<rendering::ShaderValue> converted;
        const double *number = std::get_if<double>(&value);
        switch (param->type) {
            case rendering::ShaderParamType::Int:
                if (number)
                    converted = static_cast<std::int32_t>(std::lround(*number));
                break;
            case rendering::ShaderParamType::Uint:
                if (number)
                    converted = static_cast<std::uint32_t>(std::max<long>(std::lround(*number), 0));
                break;
            case rendering::ShaderParamType::Float:
                if (number)
                    converted = static_cast<float>(*number);
                break;
            case rendering::ShaderParamType::Float2:
                if (auto array = Array<2>(value))
                    converted = *array;
                break;
            case rendering::ShaderParamType::Float3:
                if (auto array = Array<3>(value))
                    converted = *array;
                break;
            case rendering::ShaderParamType::Float4:
            case rendering::ShaderParamType::Color:
                if (auto array = Array<4>(value))
                    converted = *array;
                break;
            case rendering::ShaderParamType::Texture:
                if (const auto *texture = std::get_if<MaterialTextureRef>(&value)) {
                    rendering::MaterialTexture binding{ .filter = texture->filter, .repeat = texture->repeat };
                    m_textures.erase(name);
                    if (!texture->path.empty()) {
                        try {
                            if (auto loaded = ResourceManager::GetInstance().getResource<Texture2d>(texture->path)) {
                                binding.texture = loaded->rid();
                                m_textures[name] = std::move(loaded);
                            }
                        } catch (const std::exception &e) {
                            spdlog::error("Material texture {} could not be loaded: {}", texture->path, e.what());
                        }
                    }
                    converted = binding;
                }
                break;
        }

        if (!converted) {
            spdlog::warn("Material: parameter '{}' of {} expects a {} value", name, m_shader_path, rendering::ShaderParamTypeName(param->type));
            return false;
        }
        return rendering::RenderingServer::Instance().materialSetParam(m_rid, name, *converted);
    }

    void Material::applyAll()
    {
        for (const auto &[name, value] : m_values)
            if (m_shader->layout().find(name))
                apply(name, value);
    }
} // namespace atmo::core::resource
