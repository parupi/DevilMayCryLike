#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BlurSettings : register(b0)
{
    float sigma; // ガウス分布のσ
    float blurStrength; // ブラーの強さ倍率
    float alphaMode; // 0 = 固定1.0, 1 = サンプル結果適用
    float padding0;
    float2 uvClampMin; // UV Clamp最小値（例：0.0f, 0.0f）
    float2 uvClampMax; // UV Clamp最大値（例：1.0f, 1.0f）
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

static const float32_t PI = 3.14159265f;

float gauss(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float kernel3x3[3][3];
    float weightSum = 0.0f;

    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStep = float2(1.0f / width, 1.0f / height);

    // カーネル計算
    [unroll]
    for (int x = 0; x < 3; ++x)
    {
        [unroll]
        for (int y = 0; y < 3; ++y)
        {
            kernel3x3[x][y] = gauss(kIndex3x3[x][y].x, kIndex3x3[x][y].y, sigma);
            weightSum += kernel3x3[x][y];
        }
    }

    float3 colorSum = float3(0.0f, 0.0f, 0.0f);
    float alphaSum = 0.0f;

    [unroll]
    for (int a = 0; a < 3; ++a)
    {
        [unroll]
        for (int b = 0; b < 3; ++b)
        {
            float2 sampleUV = input.texcoord + kIndex3x3[a][b] * uvStep;
            sampleUV = clamp(sampleUV, uvClampMin, uvClampMax);
            float4 sampleColor = gTexture.Sample(gSampler, sampleUV);

            colorSum += sampleColor.rgb * kernel3x3[a][b];
            alphaSum += sampleColor.a * kernel3x3[a][b];
        }
    }

    output.color.rgb = (colorSum / weightSum) * blurStrength;
    output.color.a = (alphaMode > 0.5f) ? saturate(alphaSum / weightSum) : 1.0f;

    return output;
}