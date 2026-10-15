#include "Fullscreen.hlsli"

// SSAO のぼかし。距離が近い画素どうしだけを混ぜて、物の輪郭で AO がにじまないようにする
cbuffer SSAOBlurParam : register(b0)
{
    float gDepthSharpness; // 大きいほど、位置が離れた画素を混ぜない
    float3 _pad;
};

Texture2D<float> gAO : register(t0);
Texture2D<float4> gWorldPos : register(t1);

float main(VertexShaderOutput input) : SV_TARGET
{
    int2 pixel = int2(input.position.xy);
    float4 center = gWorldPos.Load(int3(pixel, 0));
    if (center.w < 0.5f)
    {
        return 1.0f;
    }

    float sum = 0.0f;
    float weightSum = 0.0f;
    [unroll]
    for (int y = -2; y <= 1; ++y)
    {
        [unroll]
        for (int x = -2; x <= 1; ++x)
        {
            int2 q = pixel + int2(x, y);
            float4 wq = gWorldPos.Load(int3(q, 0));
            if (wq.w < 0.5f)
                continue;
            float weight = exp(-distance(wq.xyz, center.xyz) * gDepthSharpness);
            sum += gAO.Load(int3(q, 0)) * weight;
            weightSum += weight;
        }
    }
    return weightSum > 0.0f ? sum / weightSum : 1.0f;
}
