import atmo;

public struct PostConstants
{
    public float2 target_size;
    public uint source_texture;
    public uint source_sampler;
    public float4 content;
    public float time;
    public uint material_buffer;
    public uint material_offset;
    public uint padding;
};

[[vk::push_constant]]
public ConstantBuffer<PostConstants> post_constants;

public struct PostVaryings
{
    public float4 position : SV_Position;
    public float2 uv : TEXCOORD0;
};

[shader("vertex")]
PostVaryings fullscreenVertexMain(uint vertex: SV_VertexID)
{
    float2 corner = float2(float((vertex << 1) & 2), float(vertex & 2));
    float2 pixel = post_constants.content.xy + corner * post_constants.content.zw;

    PostVaryings output;
    output.position = atmo::pixelToClip(pixel, post_constants.target_size);
    output.uv = corner;
    return output;
}

[shader("fragment")]
float4 copyFragmentMain(PostVaryings input) : SV_Target
{
    float2 uv = (post_constants.content.xy + input.uv * post_constants.content.zw) / post_constants.target_size;
    return atmo::sampleBindless(post_constants.source_texture, post_constants.source_sampler, uv);
}

public float4 viewportMaterialFragment<M : atmo::IViewportMaterial>(PostVaryings input)
{
    atmo::setFrameTime(post_constants.time);
    M material = atmo::loadBindlessBytes<M>(post_constants.material_buffer, post_constants.material_offset);

    atmo::ViewportFragment fragment;
    fragment.uv = input.uv;
    fragment.screen_position = input.position.xy;
    fragment.size = post_constants.content.zw;
    fragment.screen.index = post_constants.source_texture;
    fragment.screen.sampler_index = post_constants.source_sampler;
    fragment.content = post_constants.content;
    fragment.target_size = post_constants.target_size;
    return material.fragment(fragment);
}
