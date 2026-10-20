import atmo;

struct CanvasInstance
{
    float4 basis;
    float4 origin_uv_min;
    float4 uv_max_size;
    float4 color;
    uint4 resources;
    float4 corner_radii;
    float4 border_widths;
    float4 border_color;
};

struct CanvasVertex
{
    float4 position_uv;
    float4 color;
    uint4 resources;
};

struct CanvasConstants
{
    float2 target_size;
    uint buffer_index;
    uint first_element;
};

[[vk::push_constant]]
ConstantBuffer<CanvasConstants> constants;

struct CanvasVaryings
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    nointerpolation uint4 resources : TEXCOORD1;
    float4 local_size : TEXCOORD2;
    nointerpolation float4 corner_radii : TEXCOORD3;
    nointerpolation float4 border_widths : TEXCOORD4;
    nointerpolation float4 border_color : TEXCOORD5;
};

static const uint styled_flag = 1;

float roundedBoxDistance(float2 p, float2 half_size, float4 radii)
{
    float radius = p.x < 0.0 ? (p.y < 0.0 ? radii.x : radii.w) : (p.y < 0.0 ? radii.y : radii.z);
    float2 q = abs(p) - half_size + radius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

[shader("vertex")]
CanvasVaryings quadVertexMain(uint vertex: SV_VertexID, uint instance: SV_InstanceID)
{
    CanvasInstance item = atmo::loadBindless<CanvasInstance>(constants.buffer_index, constants.first_element + instance);
    float2 corner = float2(float(vertex & 1), float(vertex >> 1));
    float2 pixel = item.origin_uv_min.xy + corner.x * item.basis.xy + corner.y * item.basis.zw;

    CanvasVaryings output;
    output.position = atmo::pixelToClip(pixel, constants.target_size);
    output.uv = lerp(item.origin_uv_min.zw, item.uv_max_size.xy, corner);
    output.color = item.color;
    output.resources = item.resources;
    output.local_size = float4(corner * item.uv_max_size.zw, item.uv_max_size.zw);
    output.corner_radii = item.corner_radii;
    output.border_widths = item.border_widths;
    output.border_color = item.border_color;
    return output;
}

[shader("vertex")]
CanvasVaryings meshVertexMain(uint vertex: SV_VertexID)
{
    CanvasVertex point = atmo::loadBindless<CanvasVertex>(constants.buffer_index, constants.first_element + vertex);

    CanvasVaryings output;
    output.position = atmo::pixelToClip(point.position_uv.xy, constants.target_size);
    output.uv = point.position_uv.zw;
    output.color = point.color;
    output.resources = uint4(point.resources.xy, 0, 0);
    output.local_size = float4(0.0, 0.0, 0.0, 0.0);
    output.corner_radii = float4(0.0, 0.0, 0.0, 0.0);
    output.border_widths = float4(0.0, 0.0, 0.0, 0.0);
    output.border_color = float4(0.0, 0.0, 0.0, 0.0);
    return output;
}

[shader("fragment")]
float4 canvasFragmentMain(CanvasVaryings input) : SV_Target
{
    float4 fill = atmo::sampleBindless(input.resources.x, input.resources.y, input.uv) * input.color;
    float2 local_step = fwidth(input.local_size.xy);
    if ((input.resources.z & styled_flag) == 0)
        return fill;

    float2 size = input.local_size.zw;
    float2 p = input.local_size.xy;
    float aa = max(max(local_step.x, local_step.y), 1e-4);
    float4 radii = min(max(input.corner_radii, 0.0), min(size.x, size.y) * 0.5);
    float outer = saturate(0.5 - roundedBoxDistance(p - size * 0.5, size * 0.5, radii) / aa);

    float4 widths = max(input.border_widths, 0.0);
    float4 color = fill;
    if (any(widths > 0.0))
    {
        float2 inner_min = widths.xy;
        float2 inner_size = max(size - widths.zw - inner_min, 0.0);
        float4 inner_radii = max(radii - float4(max(widths.x, widths.y), max(widths.z, widths.y), max(widths.z, widths.w), max(widths.x, widths.w)), 0.0);
        float inner = 0.0;
        if (inner_size.x > 0.0 && inner_size.y > 0.0)
            inner = saturate(0.5 - roundedBoxDistance(p - (inner_min + inner_size * 0.5), inner_size * 0.5, inner_radii) / aa);

        float4 border = input.border_color;
        float alpha = lerp(border.a, fill.a, inner);
        float3 premultiplied = lerp(border.rgb * border.a, fill.rgb * fill.a, inner);
        color = float4(alpha > 0.0 ? premultiplied / alpha : float3(0.0, 0.0, 0.0), alpha);
    }

    color.a *= outer;
    return color;
}
