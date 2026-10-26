#include "shader_compiler.hpp"

#include <array>
#include <cctype>
#include <format>
#include <mutex>
#include <optional>
#include <span>
#include <string_view>

#include <slang-com-ptr.h>
#include <slang.h>

namespace atmo::server::rendering
{
    namespace
    {
        constexpr unsigned char atmo_module_source[] = {
#include "shaders/atmo.slang.h"
        };

        constexpr unsigned char canvas_module_source[] = {
#include "shaders/canvas.slang.h"
        };

        constexpr unsigned char post_module_source[] = {
#include "shaders/post.slang.h"
        };

        constexpr const char *material_entry_point = "materialFragmentMain";

        struct BuiltinModule {
            const char *name;
            const char *path;
            std::string_view source;
        };

        const std::array builtin_modules{
            BuiltinModule{ "atmo", "atmo.slang", { reinterpret_cast<const char *>(atmo_module_source), sizeof(atmo_module_source) } },
            BuiltinModule{ "canvas", "canvas.slang", { reinterpret_cast<const char *>(canvas_module_source), sizeof(canvas_module_source) } },
            BuiltinModule{ "post", "post.slang", { reinterpret_cast<const char *>(post_module_source), sizeof(post_module_source) } },
        };

        struct TargetInfo {
            SlangCompileTarget format;
            const char *profile;
        };

        std::optional<TargetInfo> Target(ShaderTarget target) noexcept
        {
            switch (target) {
                case ShaderTarget::SpirV:
                    return TargetInfo{ SLANG_SPIRV, "spirv_1_5" };
                case ShaderTarget::Msl:
                    return TargetInfo{ SLANG_METAL, "metallib_2_4" };
                case ShaderTarget::MetalLib:
                    return TargetInfo{ SLANG_METAL_LIB, "metallib_2_4" };
                case ShaderTarget::Dxil:
                    return TargetInfo{ SLANG_DXIL, "sm_6_6" };
                case ShaderTarget::None:
                    break;
            }
            return std::nullopt;
        }

        std::string BlobString(slang::IBlob *blob)
        {
            if (!blob)
                return {};
            return { static_cast<const char *>(blob->getBufferPointer()), blob->getBufferSize() };
        }

        void Append(std::string &diagnostics, slang::IBlob *blob)
        {
            std::string text = BlobString(blob);
            if (text.empty())
                return;
            if (!diagnostics.empty() && diagnostics.back() != '\n')
                diagnostics.push_back('\n');
            diagnostics += text;
        }

        std::optional<ShaderStage> Stage(SlangStage stage) noexcept
        {
            switch (stage) {
                case SLANG_STAGE_VERTEX:
                    return ShaderStage::Vertex;
                case SLANG_STAGE_FRAGMENT:
                    return ShaderStage::Fragment;
                case SLANG_STAGE_COMPUTE:
                    return ShaderStage::Compute;
                default:
                    return std::nullopt;
            }
        }

        ShaderBindingKind BindingKind(slang::TypeLayoutReflection *type)
        {
            while (type && type->getKind() == slang::TypeReflection::Kind::Array) type = type->getElementTypeLayout();
            if (!type)
                return ShaderBindingKind::Other;

            switch (type->getKind()) {
                case slang::TypeReflection::Kind::SamplerState:
                    return ShaderBindingKind::Sampler;
                case slang::TypeReflection::Kind::ConstantBuffer:
                case slang::TypeReflection::Kind::ParameterBlock:
                    return ShaderBindingKind::ConstantBuffer;
                case slang::TypeReflection::Kind::ShaderStorageBuffer:
                    return ShaderBindingKind::StorageBuffer;
                case slang::TypeReflection::Kind::Resource:
                    {
                        const SlangResourceShape shape = type->getResourceShape();
                        if ((shape & SLANG_RESOURCE_BASE_SHAPE_MASK) == SLANG_STRUCTURED_BUFFER ||
                            (shape & SLANG_RESOURCE_BASE_SHAPE_MASK) == SLANG_BYTE_ADDRESS_BUFFER)
                            return ShaderBindingKind::StorageBuffer;
                        return ShaderBindingKind::Texture;
                    }
                default:
                    return ShaderBindingKind::Other;
            }
        }

        std::uint32_t BindingCount(slang::TypeLayoutReflection *type)
        {
            std::uint32_t count = 1;
            for (; type && type->getKind() == slang::TypeReflection::Kind::Array; type = type->getElementTypeLayout()) {
                const std::size_t elements = type->getElementCount();
                if (elements == SLANG_UNBOUNDED_SIZE || elements == 0)
                    return 0;
                count *= static_cast<std::uint32_t>(elements);
            }
            return count;
        }

        std::expected<Slang::ComPtr<slang::ISession>, std::string>
        CreateSession(slang::IGlobalSession *global, ShaderTarget target, const TargetInfo &info, std::span<const std::string> search_paths = {})
        {
            std::array options{
                slang::CompilerOptionEntry{ .name = slang::CompilerOptionName::VulkanUseEntryPointName,
                                            .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 } },
                slang::CompilerOptionEntry{ .name = slang::CompilerOptionName::Optimization,
                                            .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE } },
            };

            slang::TargetDesc target_desc{};
            target_desc.format = info.format;
            target_desc.profile = global->findProfile(info.profile);

            const std::array metal_macros{ slang::PreprocessorMacroDesc{ .name = "ATMO_TARGET_METAL", .value = "1" } };
            const bool metal = target == ShaderTarget::Msl || target == ShaderTarget::MetalLib;

            slang::SessionDesc session_desc{};
            session_desc.targets = &target_desc;
            session_desc.targetCount = 1;
            session_desc.compilerOptionEntries = options.data();
            session_desc.compilerOptionEntryCount = static_cast<std::uint32_t>(options.size());
            if (metal) {
                session_desc.preprocessorMacros = metal_macros.data();
                session_desc.preprocessorMacroCount = static_cast<SlangInt>(metal_macros.size());
            }
            std::vector<const char *> paths;
            for (const std::string &path : search_paths) paths.push_back(path.c_str());
            session_desc.searchPaths = paths.data();
            session_desc.searchPathCount = static_cast<SlangInt>(paths.size());

            Slang::ComPtr<slang::ISession> session;
            if (SLANG_FAILED(global->createSession(session_desc, session.writeRef())))
                return std::unexpected("failed to create a Slang session");

            for (const BuiltinModule &module : builtin_modules) {
                Slang::ComPtr<slang::IBlob> diagnostics;
                std::string code(module.source);
                if (!session->loadModuleFromSourceString(module.name, module.path, code.c_str(), diagnostics.writeRef()))
                    return std::unexpected("failed to load the built-in Slang module '" + std::string(module.name) + "':\n" + BlobString(diagnostics));
            }
            return session;
        }

        std::string ModuleName(std::string_view name)
        {
            std::string result = "material_";
            for (char c : name) result.push_back(std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
            return result;
        }

        void CollectStructs(slang::DeclReflection *decl, const std::string &prefix, std::vector<std::pair<std::string, slang::TypeReflection *>> &out)
        {
            for (unsigned i = 0; i < decl->getChildrenCount(); ++i) {
                slang::DeclReflection *child = decl->getChild(i);
                const char *name = child->getName();
                if (!name)
                    continue;
                if (child->getKind() == slang::DeclReflection::Kind::Struct)
                    out.emplace_back(prefix + name, child->getType());
                else if (child->getKind() == slang::DeclReflection::Kind::Namespace)
                    CollectStructs(child, prefix + name + "::", out);
            }
        }

        std::optional<float> AttributeFloat(slang::UserAttribute *attribute, unsigned index)
        {
            float value = 0.0f;
            if (SLANG_SUCCEEDED(attribute->getArgumentValueFloat(index, &value)))
                return value;
            int integer = 0;
            if (SLANG_SUCCEEDED(attribute->getArgumentValueInt(index, &integer)))
                return static_cast<float>(integer);
            return std::nullopt;
        }

        slang::UserAttribute *FindAttribute(slang::IGlobalSession *global, slang::VariableReflection *field, const char *name)
        {
            if (slang::UserAttribute *attribute = field->findUserAttributeByName(global, name))
                return attribute;
            return field->findUserAttributeByName(global, (std::string(name) + "Attribute").c_str());
        }

        std::expected<ShaderParamLayout, std::string>
        ReflectParams(slang::IGlobalSession *global, slang::ProgramLayout *program, slang::TypeReflection *type, std::string_view type_name)
        {
            slang::TypeReflection *texture_type = program->findTypeByName("atmo::Texture");
            ShaderParamLayout layout;
            for (unsigned i = 0; i < type->getFieldCount(); ++i) {
                slang::VariableReflection *field = type->getFieldByIndex(i);
                slang::TypeReflection *field_type = field->getType();
                ShaderParam param{ .name = field->getName() ? field->getName() : "" };
                std::optional<ShaderParamType> param_type;

                switch (field_type->getKind()) {
                    case slang::TypeReflection::Kind::Scalar:
                        switch (field_type->getScalarType()) {
                            case slang::TypeReflection::ScalarType::Bool:
                                return std::unexpected(
                                    std::format(
                                        "parameter '{}' of '{}' is a bool, which Metal cannot load from a buffer; use a uint set to 0 or 1 instead",
                                        param.name,
                                        type_name));
                            case slang::TypeReflection::ScalarType::Int32:
                                param_type = ShaderParamType::Int;
                                break;
                            case slang::TypeReflection::ScalarType::UInt32:
                                param_type = ShaderParamType::Uint;
                                break;
                            case slang::TypeReflection::ScalarType::Float32:
                                param_type = ShaderParamType::Float;
                                break;
                            default:
                                break;
                        }
                        break;
                    case slang::TypeReflection::Kind::Vector:
                        if (field_type->getElementType() && field_type->getElementType()->getScalarType() == slang::TypeReflection::ScalarType::Float32) {
                            switch (field_type->getElementCount()) {
                                case 2:
                                    param_type = ShaderParamType::Float2;
                                    break;
                                case 3:
                                    param_type = ShaderParamType::Float3;
                                    break;
                                case 4:
                                    param_type = ShaderParamType::Float4;
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    case slang::TypeReflection::Kind::Struct:
                        if (field_type == texture_type)
                            param_type = ShaderParamType::Texture;
                        break;
                    default:
                        break;
                }

                const char *field_type_name = field_type->getName() ? field_type->getName() : "?";
                if (!param_type)
                    return std::unexpected(
                        std::format(
                            "parameter '{}' of '{}' has type '{}'; material parameters can be int, uint, float, float2, float3, float4 or atmo::Texture",
                            param.name,
                            type_name,
                            field_type_name));

                if (FindAttribute(global, field, "Color")) {
                    if (*param_type != ShaderParamType::Float4)
                        return std::unexpected(std::format("parameter '{}' of '{}' is marked [Color] but is not a float4", param.name, type_name));
                    param_type = ShaderParamType::Color;
                }
                if (slang::UserAttribute *range = FindAttribute(global, field, "Range")) {
                    std::optional<float> min = AttributeFloat(range, 0);
                    std::optional<float> max = AttributeFloat(range, 1);
                    if (!min || !max)
                        return std::unexpected(std::format("the [Range] of parameter '{}' of '{}' needs two numbers", param.name, type_name));
                    param.range = std::array{ *min, *max };
                }

                param.type = *param_type;
                param.offset = layout.size;
                layout.size += ShaderParamSize(param.type);
                layout.params.push_back(std::move(param));
            }
            return layout;
        }

        ShaderLayout Reflect(slang::ProgramLayout *program)
        {
            ShaderLayout layout;

            for (SlangUInt i = 0; i < program->getEntryPointCount(); ++i) {
                slang::EntryPointReflection *entry = program->getEntryPointByIndex(i);
                if (std::optional<ShaderStage> stage = Stage(entry->getStage()))
                    layout.entry_points.push_back({ .name = entry->getName(), .stage = *stage });
            }

            for (unsigned i = 0; i < program->getParameterCount(); ++i) {
                slang::VariableLayoutReflection *parameter = program->getParameterByIndex(i);
                slang::TypeLayoutReflection *type = parameter->getTypeLayout();

                if (parameter->getCategory() == slang::ParameterCategory::PushConstantBuffer) {
                    slang::TypeLayoutReflection *element = type->getElementTypeLayout();
                    layout.push_constant_size += static_cast<std::uint32_t>(element ? element->getSize() : type->getSize());
                    continue;
                }

                if (parameter->getCategory() != slang::ParameterCategory::DescriptorTableSlot)
                    continue;

                layout.bindings.push_back(
                    {
                        .name = parameter->getName() ? parameter->getName() : "",
                        .set = static_cast<std::uint32_t>(parameter->getBindingSpace()),
                        .binding = static_cast<std::uint32_t>(parameter->getBindingIndex()),
                        .kind = BindingKind(type),
                        .count = BindingCount(type),
                    });
            }

            return layout;
        }
    } // namespace

    ShaderBlob CompiledShader::blob() const noexcept
    {
        return { .target = target, .code = code };
    }

    struct ShaderCompiler::Impl {
        std::mutex mutex;
        Slang::ComPtr<slang::IGlobalSession> global_session;

        slang::IGlobalSession *globalSession()
        {
            if (!global_session)
                slang::createGlobalSession(global_session.writeRef());
            return global_session.get();
        }
    };

    ShaderCompiler::ShaderCompiler() : m_impl(std::make_unique<Impl>()) {}

    ShaderCompiler::~ShaderCompiler() = default;

    bool ShaderCompiler::Supports(ShaderTarget target) noexcept
    {
        return Target(target).has_value();
    }

    std::expected<CompiledShader, std::string> ShaderCompiler::compile(const ShaderSource &source, ShaderTarget target)
    {
        std::optional<TargetInfo> info = Target(target);
        if (!info)
            return std::unexpected("the shader target is not supported by the shader compiler");

        std::scoped_lock lock(m_impl->mutex);
        slang::IGlobalSession *global = m_impl->globalSession();
        if (!global)
            return std::unexpected("failed to create the Slang global session");

        auto created = CreateSession(global, target, *info, source.search_paths);
        if (!created)
            return std::unexpected(created.error());
        Slang::ComPtr<slang::ISession> session = *created;

        CompiledShader result{ .target = target };

        Slang::ComPtr<slang::IBlob> diagnostics;
        const std::string path = source.path.empty() ? source.name + ".slang" : source.path;
        slang::IModule *module = session->loadModuleFromSourceString(source.name.c_str(), path.c_str(), source.code.c_str(), diagnostics.writeRef());
        Append(result.diagnostics, diagnostics);
        if (!module)
            return std::unexpected(result.diagnostics.empty() ? "failed to compile the shader" : result.diagnostics);

        std::vector<slang::IComponentType *> components{ module };
        std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points(static_cast<std::size_t>(module->getDefinedEntryPointCount()));
        for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); ++i) {
            if (SLANG_FAILED(module->getDefinedEntryPoint(i, entry_points[static_cast<std::size_t>(i)].writeRef())))
                return std::unexpected("failed to query the shader entry points");
            components.push_back(entry_points[static_cast<std::size_t>(i)].get());
        }
        if (entry_points.empty())
            return std::unexpected("the shader defines no entry points");

        Slang::ComPtr<slang::IComponentType> composite;
        diagnostics = nullptr;
        if (SLANG_FAILED(session->createCompositeComponentType(
                components.data(), static_cast<SlangInt>(components.size()), composite.writeRef(), diagnostics.writeRef()))) {
            Append(result.diagnostics, diagnostics);
            return std::unexpected(result.diagnostics);
        }

        Slang::ComPtr<slang::IComponentType> linked;
        diagnostics = nullptr;
        if (SLANG_FAILED(composite->link(linked.writeRef(), diagnostics.writeRef()))) {
            Append(result.diagnostics, diagnostics);
            return std::unexpected(result.diagnostics);
        }

        Slang::ComPtr<slang::IBlob> code;
        diagnostics = nullptr;
        const SlangResult status = linked->getTargetCode(0, code.writeRef(), diagnostics.writeRef());
        Append(result.diagnostics, diagnostics);
        if (SLANG_FAILED(status) || !code)
            return std::unexpected(result.diagnostics.empty() ? "failed to generate the shader code" : result.diagnostics);

        const auto *bytes = static_cast<const std::byte *>(code->getBufferPointer());
        result.code.assign(bytes, bytes + code->getBufferSize());

        diagnostics = nullptr;
        slang::ProgramLayout *program = linked->getLayout(0, diagnostics.writeRef());
        Append(result.diagnostics, diagnostics);
        if (!program)
            return std::unexpected(result.diagnostics.empty() ? "failed to reflect the shader" : result.diagnostics);
        result.layout = Reflect(program);

        return result;
    }

    std::expected<CompiledMaterial, std::string> ShaderCompiler::compileMaterial(const ShaderSource &source, std::span<const ShaderTarget> targets)
    {
        if (targets.empty())
            return std::unexpected("no shader target was requested");

        std::scoped_lock lock(m_impl->mutex);
        slang::IGlobalSession *global = m_impl->globalSession();
        if (!global)
            return std::unexpected("failed to create the Slang global session");

        CompiledMaterial result;
        result.desc.name = source.name;
        result.desc.entry_point = material_entry_point;
        const std::string module_name = ModuleName(source.name);
        const std::string path = source.path.empty() ? source.name + ".slang" : source.path;

        for (ShaderTarget target : targets) {
            std::optional<TargetInfo> info = Target(target);
            if (!info)
                return std::unexpected("the shader target is not supported by the shader compiler");

            auto created = CreateSession(global, target, *info, source.search_paths);
            if (!created)
                return std::unexpected(created.error());
            Slang::ComPtr<slang::ISession> session = *created;

            Slang::ComPtr<slang::IBlob> diagnostics;
            slang::IModule *module = session->loadModuleFromSourceString(module_name.c_str(), path.c_str(), source.code.c_str(), diagnostics.writeRef());
            Append(result.diagnostics, diagnostics);
            if (!module)
                return std::unexpected(result.diagnostics.empty() ? "failed to compile the shader" : result.diagnostics);

            if (result.type_name.empty()) {
                diagnostics = nullptr;
                slang::ProgramLayout *program = module->getLayout(0, diagnostics.writeRef());
                if (!program)
                    return std::unexpected("failed to reflect the shader:\n" + BlobString(diagnostics));

                slang::TypeReflection *canvas = program->findTypeByName("atmo::ICanvasMaterial");
                slang::TypeReflection *viewport = program->findTypeByName("atmo::IViewportMaterial");
                std::vector<std::pair<std::string, slang::TypeReflection *>> structs;
                CollectStructs(module->getModuleReflection(), "", structs);

                std::vector<std::pair<std::string, ShaderKind>> materials;
                std::vector<slang::TypeReflection *> material_types;
                for (const auto &[name, type] : structs) {
                    if (!type)
                        continue;
                    if (canvas && program->isSubType(type, canvas)) {
                        materials.emplace_back(name, ShaderKind::Canvas);
                        material_types.push_back(type);
                    } else if (viewport && program->isSubType(type, viewport)) {
                        materials.emplace_back(name, ShaderKind::Viewport);
                        material_types.push_back(type);
                    }
                }

                if (materials.size() != 1) {
                    std::string names;
                    for (const auto &[name, kind] : materials) names += (names.empty() ? "" : ", ") + name;
                    if (materials.empty())
                        return std::unexpected(std::format("'{}' defines no struct that implements atmo::ICanvasMaterial or atmo::IViewportMaterial", path));
                    return std::unexpected(std::format("'{}' must define exactly one material, found {}", path, names));
                }

                auto layout = ReflectParams(global, program, material_types.front(), materials.front().first);
                if (!layout)
                    return std::unexpected(layout.error());
                result.type_name = materials.front().first;
                result.desc.kind = materials.front().second;
                result.desc.layout = std::move(*layout);
            }

            const bool canvas = result.desc.kind == ShaderKind::Canvas;
            const std::string entry_source = std::format(
                "import atmo;\nimport {};\nimport {};\n[shader(\"fragment\")]\nfloat4 {}({} input) : SV_Target\n{{\n    return {}<{}>(input);\n}}\n",
                canvas ? "canvas" : "post",
                module_name,
                material_entry_point,
                canvas ? "CanvasVaryings" : "PostVaryings",
                canvas ? "canvasMaterialFragment" : "viewportMaterialFragment",
                result.type_name);

            diagnostics = nullptr;
            slang::IModule *entry_module =
                session->loadModuleFromSourceString("material_entry", "material_entry.slang", entry_source.c_str(), diagnostics.writeRef());
            Append(result.diagnostics, diagnostics);
            if (!entry_module)
                return std::unexpected(result.diagnostics.empty() ? "failed to build the material entry point" : result.diagnostics);

            Slang::ComPtr<slang::IEntryPoint> entry_point;
            if (SLANG_FAILED(entry_module->findEntryPointByName(material_entry_point, entry_point.writeRef())))
                return std::unexpected("failed to find the material entry point");

            std::array<slang::IComponentType *, 2> components{ entry_module, entry_point.get() };
            Slang::ComPtr<slang::IComponentType> composite;
            diagnostics = nullptr;
            if (SLANG_FAILED(session->createCompositeComponentType(
                    components.data(), static_cast<SlangInt>(components.size()), composite.writeRef(), diagnostics.writeRef()))) {
                Append(result.diagnostics, diagnostics);
                return std::unexpected(result.diagnostics);
            }

            Slang::ComPtr<slang::IComponentType> linked;
            diagnostics = nullptr;
            if (SLANG_FAILED(composite->link(linked.writeRef(), diagnostics.writeRef()))) {
                Append(result.diagnostics, diagnostics);
                return std::unexpected(result.diagnostics);
            }

            Slang::ComPtr<slang::IBlob> code;
            diagnostics = nullptr;
            const SlangResult status = linked->getTargetCode(0, code.writeRef(), diagnostics.writeRef());
            Append(result.diagnostics, diagnostics);
            if (SLANG_FAILED(status) || !code)
                return std::unexpected(result.diagnostics.empty() ? "failed to generate the material code" : result.diagnostics);

            const auto *bytes = static_cast<const std::byte *>(code->getBufferPointer());
            result.desc.code.push_back({ .target = target, .code = std::vector<std::byte>(bytes, bytes + code->getBufferSize()) });
        }

        return result;
    }
} // namespace atmo::server::rendering
