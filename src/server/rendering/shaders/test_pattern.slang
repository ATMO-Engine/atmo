import atmo;

struct TestPatternConstants
{
    float4 rect;
    float4 color;
    uint texture_index;
    uint sampler_index;
};

[[vk::push_constant]]
ConstantBuffer<TestPatternConstants> constants;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertex: SV_VertexID)
{
    float2 corner = float2(float(vertex & 1), float(vertex >> 1));

    VertexOutput output;
    output.position = float4(constants.rect.xy + corner * constants.rect.zw, 0.0, 1.0);
    output.uv = corner;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return atmo::sampleBindless(constants.texture_index, constants.sampler_index, input.uv) * constants.color;
}
