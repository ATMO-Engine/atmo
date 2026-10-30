module atmo;

[__AttributeUsage(_AttributeTargets.Var)]
public struct RangeAttribute
{
    public float min;
    public float max;
};

[__AttributeUsage(_AttributeTargets.Var)]
public struct ColorAttribute
{
};

namespace atmo
{
#if defined(ATMO_TARGET_METAL)
    public struct BindlessTable
    {
        public Texture2D textures[16384];
        public SamplerState samplers[64];
        public ByteAddressBuffer buffers[4096];
    };

    [[vk::binding(0, 0)]]
    public ParameterBlock<BindlessTable> bindless : register(b30);

    public float4 sampleBindless(uint texture_index, uint sampler_index, float2 uv)
    {
        return bindless.textures[texture_index].Sample(bindless.samplers[sampler_index], uv);
    }

    public T loadBindless<T>(uint buffer_index, uint element)
    {
        return bindless.buffers[buffer_index].Load<T>(element * sizeof(T));
    }

    public T loadBindlessBytes<T>(uint buffer_index, uint offset)
    {
        return bindless.buffers[buffer_index].Load<T>(offset);
    }
#else
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

    public T loadBindlessBytes<T>(uint buffer_index, uint offset)
    {
        return buffers[buffer_index].Load<T>(offset);
    }
#endif

    public float4 pixelToClip(float2 pixel, float2 target_size)
    {
        return float4((pixel.x / target_size.x) * 2.0 - 1.0, 1.0 - (pixel.y / target_size.y) * 2.0, 0.0, 1.0);
    }

    static float g_time;

    public float time()
    {
        return g_time;
    }

    public void setFrameTime(float seconds)
    {
        g_time = seconds;
    }

    public struct Texture
    {
        public uint index;
        public uint sampler_index;

        public float4 sample(float2 uv)
        {
            return sampleBindless(index, sampler_index, uv);
        }
    };

    public struct CanvasFragment
    {
        public float2 uv;
        public float4 color;
        public float2 position;
        public float2 screen_position;
        public float4 custom;
        public Texture image;

        public float4 sample(float2 uv)
        {
            return image.sample(uv);
        }
    };

    public interface ICanvasMaterial
    {
        public float4 fragment(CanvasFragment input);
    };

    public struct ViewportFragment
    {
        public float2 uv;
        public float2 screen_position;
        public float2 size;
        public Texture screen;
        public float4 content;
        public float2 target_size;

        public float4 sample(float2 uv)
        {
            return screen.sample((content.xy + uv * content.zw) / target_size);
        }
    };

    public interface IViewportMaterial
    {
        public float4 fragment(ViewportFragment input);
    };
}
