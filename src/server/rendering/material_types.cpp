#include "material_types.hpp"

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace atmo::server::rendering
{
    namespace
    {
        constexpr std::array param_type_names{ "int", "uint", "float", "float2", "float3", "float4", "color", "texture" };

        template <typename T> void Store(MaterialData &data, std::uint32_t offset, const T &value)
        {
            std::memcpy(data.bytes.data() + offset, &value, sizeof(T));
        }

        template <typename T> std::optional<double> Scalar(const T &value)
        {
            if constexpr (std::is_same_v<T, bool>)
                return value ? 1.0 : 0.0;
            else if constexpr (std::is_arithmetic_v<T>)
                return static_cast<double>(value);
            else
                return std::nullopt;
        }

        template <std::size_t N> std::optional<std::array<float, N>> Vector(const ShaderValue &value)
        {
            if (const auto *vector = std::get_if<std::array<float, N>>(&value))
                return *vector;
            return std::nullopt;
        }
    } // namespace

    std::uint32_t ShaderParamSize(ShaderParamType type) noexcept
    {
        switch (type) {
            case ShaderParamType::Int:
            case ShaderParamType::Uint:
            case ShaderParamType::Float:
                return 4;
            case ShaderParamType::Float2:
            case ShaderParamType::Texture:
                return 8;
            case ShaderParamType::Float3:
                return 12;
            case ShaderParamType::Float4:
            case ShaderParamType::Color:
                return 16;
        }
        return 0;
    }

    std::string_view ShaderParamTypeName(ShaderParamType type) noexcept
    {
        return param_type_names[static_cast<std::size_t>(type)];
    }

    std::optional<ShaderParamType> ParseShaderParamType(std::string_view name) noexcept
    {
        for (std::size_t i = 0; i < param_type_names.size(); ++i)
            if (name == param_type_names[i])
                return static_cast<ShaderParamType>(i);
        return std::nullopt;
    }

    std::string_view ShaderKindName(ShaderKind kind) noexcept
    {
        return kind == ShaderKind::Viewport ? "viewport" : "canvas";
    }

    std::optional<ShaderKind> ParseShaderKind(std::string_view name) noexcept
    {
        if (name == "canvas")
            return ShaderKind::Canvas;
        if (name == "viewport")
            return ShaderKind::Viewport;
        return std::nullopt;
    }

    const ShaderParam *ShaderParamLayout::find(std::string_view name) const noexcept
    {
        auto it = std::ranges::find(params, name, &ShaderParam::name);
        return it == params.end() ? nullptr : &*it;
    }

    const ShaderCode *ShaderDesc::codeFor(ShaderTarget target) const noexcept
    {
        auto it = std::ranges::find(code, target, &ShaderCode::target);
        return it == code.end() ? nullptr : &*it;
    }

    bool WriteShaderValue(const ShaderParam &param, const ShaderValue &value, MaterialData &data)
    {
        if (param.offset + ShaderParamSize(param.type) > data.bytes.size())
            return false;

        const std::optional<double> scalar = std::visit([](const auto &v) { return Scalar(v); }, value);
        switch (param.type) {
            case ShaderParamType::Int:
                if (!scalar)
                    return false;
                Store(data, param.offset, static_cast<std::int32_t>(*scalar));
                return true;
            case ShaderParamType::Uint:
                if (!scalar)
                    return false;
                Store(data, param.offset, static_cast<std::uint32_t>(std::max(*scalar, 0.0)));
                return true;
            case ShaderParamType::Float:
                if (!scalar)
                    return false;
                Store(data, param.offset, static_cast<float>(*scalar));
                return true;
            case ShaderParamType::Float2:
                if (auto vector = Vector<2>(value)) {
                    Store(data, param.offset, *vector);
                    return true;
                }
                return false;
            case ShaderParamType::Float3:
                if (auto vector = Vector<3>(value)) {
                    Store(data, param.offset, *vector);
                    return true;
                }
                return false;
            case ShaderParamType::Float4:
            case ShaderParamType::Color:
                if (auto vector = Vector<4>(value)) {
                    Store(data, param.offset, *vector);
                    return true;
                }
                return false;
            case ShaderParamType::Texture:
                if (const auto *texture = std::get_if<MaterialTexture>(&value)) {
                    std::erase_if(data.textures, [&param](const MaterialTextureBinding &binding) { return binding.offset == param.offset; });
                    Store<std::array<std::uint32_t, 2>>(data, param.offset, {});
                    data.textures.push_back({ .offset = param.offset, .texture = *texture });
                    return true;
                }
                return false;
        }
        return false;
    }
} // namespace atmo::server::rendering
