#include "metal_utils.hpp"

#include <format>

namespace atmo::server::rendering::metal
{
    MetalError::MetalError(std::string_view call, const NS::Error *error) : std::runtime_error(std::format("{} failed: {}", call, ErrorString(error))) {}

    NS::String *ToNsString(std::string_view text)
    {
        const std::string owned(text);
        NS::String *string = NS::String::string(owned.c_str(), NS::UTF8StringEncoding);
        return string ? string : NS::String::string();
    }

    std::string ErrorString(const NS::Error *error)
    {
        if (!error)
            return "unknown error";
        const NS::String *description = error->localizedDescription();
        return description && description->utf8String() ? description->utf8String() : "unknown error";
    }

    MTL::PixelFormat ToMtlFormat(TextureFormat format) noexcept
    {
        switch (format) {
            case TextureFormat::Undefined:
                return MTL::PixelFormatInvalid;
            case TextureFormat::R8Unorm:
                return MTL::PixelFormatR8Unorm;
            case TextureFormat::Rgba8Unorm:
                return MTL::PixelFormatRGBA8Unorm;
            case TextureFormat::Rgba8Srgb:
                return MTL::PixelFormatRGBA8Unorm_sRGB;
            case TextureFormat::Bgra8Unorm:
                return MTL::PixelFormatBGRA8Unorm;
            case TextureFormat::Bgra8Srgb:
                return MTL::PixelFormatBGRA8Unorm_sRGB;
        }
        return MTL::PixelFormatInvalid;
    }

    TextureFormat FromMtlFormat(MTL::PixelFormat format) noexcept
    {
        switch (format) {
            case MTL::PixelFormatR8Unorm:
                return TextureFormat::R8Unorm;
            case MTL::PixelFormatRGBA8Unorm:
                return TextureFormat::Rgba8Unorm;
            case MTL::PixelFormatRGBA8Unorm_sRGB:
                return TextureFormat::Rgba8Srgb;
            case MTL::PixelFormatBGRA8Unorm:
                return TextureFormat::Bgra8Unorm;
            case MTL::PixelFormatBGRA8Unorm_sRGB:
                return TextureFormat::Bgra8Srgb;
            default:
                return TextureFormat::Undefined;
        }
    }

    MTL::TextureUsage ToMtlTextureUsage(TextureUsage usage) noexcept
    {
        MTL::TextureUsage result = MTL::TextureUsageUnknown;
        if (HasFlag(usage, TextureUsage::Sampled))
            result |= MTL::TextureUsageShaderRead;
        if (HasFlag(usage, TextureUsage::RenderTarget))
            result |= MTL::TextureUsageRenderTarget;
        return result;
    }

    MTL::SamplerMinMagFilter ToMtlFilter(Filter filter) noexcept
    {
        return filter == Filter::Nearest ? MTL::SamplerMinMagFilterNearest : MTL::SamplerMinMagFilterLinear;
    }

    MTL::SamplerAddressMode ToMtlAddressMode(AddressMode mode) noexcept
    {
        switch (mode) {
            case AddressMode::ClampToEdge:
                return MTL::SamplerAddressModeClampToEdge;
            case AddressMode::Repeat:
                return MTL::SamplerAddressModeRepeat;
            case AddressMode::MirroredRepeat:
                return MTL::SamplerAddressModeMirrorRepeat;
        }
        return MTL::SamplerAddressModeClampToEdge;
    }

    MTL::PrimitiveType ToMtlPrimitive(PrimitiveTopology topology) noexcept
    {
        switch (topology) {
            case PrimitiveTopology::TriangleList:
                return MTL::PrimitiveTypeTriangle;
            case PrimitiveTopology::TriangleStrip:
                return MTL::PrimitiveTypeTriangleStrip;
            case PrimitiveTopology::LineList:
                return MTL::PrimitiveTypeLine;
            case PrimitiveTopology::LineStrip:
                return MTL::PrimitiveTypeLineStrip;
        }
        return MTL::PrimitiveTypeTriangle;
    }

    MTL::PrimitiveTopologyClass ToMtlTopologyClass(PrimitiveTopology topology) noexcept
    {
        switch (topology) {
            case PrimitiveTopology::TriangleList:
            case PrimitiveTopology::TriangleStrip:
                return MTL::PrimitiveTopologyClassTriangle;
            case PrimitiveTopology::LineList:
            case PrimitiveTopology::LineStrip:
                return MTL::PrimitiveTopologyClassLine;
        }
        return MTL::PrimitiveTopologyClassTriangle;
    }

    MTL::LoadAction ToMtlLoadAction(LoadOp op) noexcept
    {
        switch (op) {
            case LoadOp::Load:
                return MTL::LoadActionLoad;
            case LoadOp::Clear:
                return MTL::LoadActionClear;
            case LoadOp::DontCare:
                return MTL::LoadActionDontCare;
        }
        return MTL::LoadActionClear;
    }

    MTL::StoreAction ToMtlStoreAction(StoreOp op) noexcept
    {
        return op == StoreOp::Store ? MTL::StoreActionStore : MTL::StoreActionDontCare;
    }

    void ApplyBlend(MTL::RenderPipelineColorAttachmentDescriptor *attachment, BlendMode mode) noexcept
    {
        attachment->setBlendingEnabled(mode != BlendMode::Opaque);
        attachment->setRgbBlendOperation(MTL::BlendOperationAdd);
        attachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
        attachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
        attachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
        attachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
        attachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

        switch (mode) {
            case BlendMode::Opaque:
            case BlendMode::Alpha:
                break;
            case BlendMode::PremultipliedAlpha:
                attachment->setSourceRGBBlendFactor(MTL::BlendFactorOne);
                break;
            case BlendMode::Additive:
                attachment->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
                attachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
                break;
        }
    }
} // namespace atmo::server::rendering::metal
