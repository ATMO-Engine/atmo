#include "shader_compiler.hpp"

#include <array>
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

        struct BuiltinModule {
            const char *name;
            const char *path;
            std::string_view source;
        };

        const std::array builtin_modules{
            BuiltinModule{ "atmo", "atmo.slang", { reinterpret_cast<const char *>(atmo_module_source), sizeof(atmo_module_source) } },
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

        std::array options{
            slang::CompilerOptionEntry{ .name = slang::CompilerOptionName::VulkanUseEntryPointName,
                                        .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 } },
            slang::CompilerOptionEntry{ .name = slang::CompilerOptionName::Optimization,
                                        .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE } },
        };

        slang::TargetDesc target_desc{};
        target_desc.format = info->format;
        target_desc.profile = global->findProfile(info->profile);

        slang::SessionDesc session_desc{};
        session_desc.targets = &target_desc;
        session_desc.targetCount = 1;
        session_desc.compilerOptionEntries = options.data();
        session_desc.compilerOptionEntryCount = static_cast<std::uint32_t>(options.size());

        Slang::ComPtr<slang::ISession> session;
        if (SLANG_FAILED(global->createSession(session_desc, session.writeRef())))
            return std::unexpected("failed to create a Slang session");

        CompiledShader result{ .target = target };

        for (const BuiltinModule &module : builtin_modules) {
            Slang::ComPtr<slang::IBlob> diagnostics;
            std::string code(module.source);
            if (!session->loadModuleFromSourceString(module.name, module.path, code.c_str(), diagnostics.writeRef()))
                return std::unexpected("failed to load the built-in Slang module '" + std::string(module.name) + "':\n" + BlobString(diagnostics));
        }

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
} // namespace atmo::server::rendering
