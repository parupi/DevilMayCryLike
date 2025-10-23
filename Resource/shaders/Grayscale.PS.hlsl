#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

cbuffer GrayScaleParam : register(b0)
{
    float32_t intensity;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャカラー取得
    float4 color = gTexture.Sample(gSampler, input.texcoord);

    // グレースケール値を計算
    float gray = dot(color.rgb, float3(0.2125f, 0.7154f, 0.0721f));

    // 線形補間でグレースケールと元の色を混ぜる
    float3 blended = lerp(color.rgb, float3(gray, gray, gray), intensity);

    output.color = float4(blended, color.a);
    return output;
}