#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "core/resource/shader.hpp"
#include "core/resource/texture_2d.hpp"
#include "server/rendering/material_types.hpp"
#include "server/rendering/rid.hpp"

namespace atmo::core::resource
{
    struct MaterialTextureRef {
        std::string path;
        server::rendering::TextureFilter filter = server::rendering::TextureFilter::Linear;
        bool repeat = false;

        bool operator==(const MaterialTextureRef &) const = default;
    };

    using MaterialValue = std::variant<double, std::array<float, 2>, std::array<float, 3>, std::array<float, 4>, MaterialTextureRef>;

    class Material
    {
    public:
        static constexpr std::string_view extension = ".material";

        Material() = default;
        ~Material();

        Material(const Material &) = delete;
        Material &operator=(const Material &) = delete;

        [[nodiscard]] static std::shared_ptr<Material> FromJson(std::string_view json, std::string &error);
        [[nodiscard]] std::string toJson() const;
        bool save(const std::string &path) const;

        bool setShader(const std::string &shader_path);
        [[nodiscard]] const std::string &shaderPath() const noexcept;
        [[nodiscard]] const std::shared_ptr<Shader> &shader() const noexcept;

        [[nodiscard]] server::rendering::MaterialRid rid();
        [[nodiscard]] std::optional<server::rendering::ShaderKind> kind() const noexcept;

        bool set(std::string_view name, MaterialValue value);
        [[nodiscard]] std::optional<MaterialValue> get(std::string_view name) const;
        [[nodiscard]] const std::vector<std::pair<std::string, MaterialValue>> &values() const noexcept;

        [[nodiscard]] static MaterialValue DefaultValue(const server::rendering::ShaderParam &param);

    private:
        bool apply(const std::string &name, const MaterialValue &value);
        void applyAll();

        std::string m_shader_path;
        std::shared_ptr<Shader> m_shader;
        std::uint64_t m_shader_version = 0;
        server::rendering::MaterialRid m_rid;
        std::vector<std::pair<std::string, MaterialValue>> m_values;
        std::unordered_map<std::string, std::shared_ptr<Texture2d>> m_textures;
    };
} // namespace atmo::core::resource
