#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

cbuffer GrayScaleParam : register(b0)
{
    float32_t blurStrength;
    int32_t iterations;
}

static const float32_t2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};

static const float32_t kKernel3x3[3][3] =
{
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f },
    { 1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f }
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 currentColor = gTexture.Sample(gSampler, input.texcoord);

    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 baseStep = float2(1.0f / width, 1.0f / height) * blurStrength;

    float3 resultColor = float3(0.0f, 0.0f, 0.0f);

    // 繰り返しぼかし処理
    for (int i = 0; i < iterations; ++i)
    {
        float3 sumColor = float3(0.0f, 0.0f, 0.0f);
        for (int x = 0; x < 3; ++x)
        {
            for (int y = 0; y < 3; ++y)
            {
                float2 offset = kIndex3x3[x][y] * baseStep;
                float2 coord = input.texcoord + offset;
                float3 sample = gTexture.Sample(gSampler, coord).rgb;
                sumColor += sample * kKernel3x3[x][y];
            }
        }
        resultColor = lerp(resultColor, sumColor, 1.0f / (i + 1)); // 徐々に平均化
    }

    output.color.rgb = resultColor;
    output.color.a = currentColor.a; // 元のアルファ維持

    return output;
}