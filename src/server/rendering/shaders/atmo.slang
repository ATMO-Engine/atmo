module atmo;

namespace atmo
{
    [[vk::binding(0, 0)]]
    public Texture2D textures[];

    [[vk::binding(1, 0)]]
    public SamplerState samplers[];

    [[vk::binding(2, 0)]]
    public ByteAddressBuffer buffers[];

    public float4 sampleBindless(uint texture_index, uint sampler_index, float2 uv)
    {
        return textures[NonUniformResourceIndex(texture_index)].Sample(samplers[NonUniformResourceIndex(sampler_index)], uv);
    }

    public T loadBindless<T>(uint buffer_index, uint element)
    {
        return buffers[buffer_index].Load<T>(element * sizeof(T));
    }

    public float4 pixelToClip(float2 pixel, float2 target_size)
    {
        return float4((pixel.x / target_size.x) * 2.0 - 1.0, 1.0 - (pixel.y / target_size.y) * 2.0, 0.0, 1.0);
    }
}
