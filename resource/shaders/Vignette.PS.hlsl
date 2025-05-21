#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

cbuffer VignetteScaleParam : register(b0)
{
    float32_t radius;
    float32_t intensity;
    float32_t softness;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);

    // テクスチャ中心からの距離を計算
    float2 center = float2(0.5f, 0.5f);
    float dist = distance(input.texcoord, center);

    // 距離に基づいてビネット値を計算
    float vignette = smoothstep(radius, radius - softness, dist);
    vignette = 1.0f - vignette; // 中心を1、外側を0に

    // 強度を適用
    vignette = lerp(1.0f, vignette, intensity);

    // 色に乗算
    output.color.rgb = texColor.rgb * vignette;
    output.color.a = texColor.a;

    return output;
}