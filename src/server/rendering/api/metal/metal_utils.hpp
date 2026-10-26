#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include "metal_includes.hpp"
#include "server/rendering/platform/rendering_platform.hpp"

namespace atmo::server::rendering::metal
{
    class MetalError : public std::runtime_error
    {
    public:
        MetalError(std::string_view call, const NS::Error *error);
    };

    [[nodiscard]] NS::String *ToNsString(std::string_view text);
    [[nodiscard]] std::string ErrorString(const NS::Error *error);

    [[nodiscard]] MTL::PixelFormat ToMtlFormat(TextureFormat format) noexcept;
    [[nodiscard]] TextureFormat FromMtlFormat(MTL::PixelFormat format) noexcept;
    [[nodiscard]] MTL::TextureUsage ToMtlTextureUsage(TextureUsage usage) noexcept;
    [[nodiscard]] MTL::SamplerMinMagFilter ToMtlFilter(Filter filter) noexcept;
    [[nodiscard]] MTL::SamplerAddressMode ToMtlAddressMode(AddressMode mode) noexcept;
    [[nodiscard]] MTL::PrimitiveType ToMtlPrimitive(PrimitiveTopology topology) noexcept;
    [[nodiscard]] MTL::PrimitiveTopologyClass ToMtlTopologyClass(PrimitiveTopology topology) noexcept;
    [[nodiscard]] MTL::LoadAction ToMtlLoadAction(LoadOp op) noexcept;
    [[nodiscard]] MTL::StoreAction ToMtlStoreAction(StoreOp op) noexcept;
    void ApplyBlend(MTL::RenderPipelineColorAttachmentDescriptor *attachment, BlendMode mode) noexcept;
} // namespace atmo::server::rendering::metal
