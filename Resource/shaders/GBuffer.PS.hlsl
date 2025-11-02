// GBufferPS.hlsl
#include "GBuffer.hlsli"

Texture2D baseColorMap : register(t0);
SamplerState samLinear : register(s0);

// Material param: Roughness / Metal
cbuffer MaterialParam : register(b0)
{
    float roughness;
    float metal;
}

GBufferOutput main(VSOutput input)
{
    GBufferOutput o;

    float4 baseColor = baseColorMap.Sample(samLinear, input.uv);

    o.baseColor_Roughness = float4(baseColor.rgb, roughness);
    o.normal_Metal = float4(normalize(input.normalVS) * 0.5f + 0.5f, metal);

    // depth (or linear depth)　好きなパターン
    o.depthDummy = float4(input.positionCS.z / input.positionCS.w, 0, 0, 0);

    return o;
}
