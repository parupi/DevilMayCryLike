#include "Fullscreen.hlsli"

Texture2D<float4> gLighting : register(t0);
SamplerState gSampler : register(s0);

// 写し方（FinalCompositePipeline の b0）
cbuffer CopyParam : register(b0)
{
    float gClampMode; // 0 = そのまま（HDR のまま / 最終出力）, 1 = 0〜1 に切り詰める（トーンマップを使わないシーン）
    float3 _pad;
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 color = gLighting.Sample(gSampler, input.texcoord);
    if (gClampMode > 0.5f)
    {
        color.rgb = saturate(color.rgb);
    }
    return color;
}
