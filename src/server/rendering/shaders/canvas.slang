import atmo;

struct CanvasInstance
{
    float4 basis;
    float4 origin_uv_min;
    float4 uv_max;
    float4 color;
    uint4 resources;
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
    nointerpolation uint2 resources : TEXCOORD1;
};

[shader("vertex")]
CanvasVaryings quadVertexMain(uint vertex: SV_VertexID, uint instance: SV_InstanceID)
{
    CanvasInstance item = atmo::loadBindless<CanvasInstance>(constants.buffer_index, constants.first_element + instance);
    float2 corner = float2(float(vertex & 1), float(vertex >> 1));
    float2 pixel = item.origin_uv_min.xy + corner.x * item.basis.xy + corner.y * item.basis.zw;

    CanvasVaryings output;
    output.position = atmo::pixelToClip(pixel, constants.target_size);
    output.uv = lerp(item.origin_uv_min.zw, item.uv_max.xy, corner);
    output.color = item.color;
    output.resources = item.resources.xy;
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
    output.resources = point.resources.xy;
    return output;
}

[shader("fragment")]
float4 canvasFragmentMain(CanvasVaryings input) : SV_Target
{
    return atmo::sampleBindless(input.resources.x, input.resources.y, input.uv) * input.color;
}
