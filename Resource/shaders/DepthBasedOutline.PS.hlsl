#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);
SamplerState gSamplerPoint : register(s0);
SamplerState gSampler : register(s1);

cbuffer OutlineParam : register(b0)
{
    float32_t projectionInverse;
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};

static const float32_t kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float32_t kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

float32_t Luminance(float32_t3 v)
{
    return dot(v, float32_t3(0.2125f, 0.7154f, 0.0721f));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(1.0f / width, 1.0f / height);
    
    float32_t2 difference = float32_t2(0.0f, 0.0f); // 縦横それぞれの畳み込みの結果を格納する
    
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            
            float32_t ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);
            float32_t4 viewSpace = mul(float32_4(0.0f, 0.0f, ndcDepth, 1.0f), projectionInverse);
            float32_t viewZ = viewSpace.z * rcp(viewSpace.w);
            difference.x += ViewZ * kPrewittHorizontalKernel[x][y];
            difference.y += ViewZ * kPrewittVerticalKernel[x][y];
            
            //float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            //float32_t luminance = Luminance(fetchColor);
            //difference.x += luminance * kPrewittHorizontalKernel[x][y];
            //difference.y += luminance * kPrewittVerticalKernel[x][y];
        }
    }
    
    float32_t weight = length(difference);
    // 差が小さすぎてわかりづらいので数倍にする
    weight = saturate(weight);
    
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;
    return output;
}