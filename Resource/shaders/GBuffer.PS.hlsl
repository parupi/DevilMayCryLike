#include "GBuffer.hlsli"

Texture2D baseColorMap : register(t0);
SamplerState samLinear : register(s0);

cbuffer MaterialParam : register(b0)
{
    float roughness;
    float metal;
    float2 padding;
    
    float4x4 uvTransform;
};

GBufferOutput main(VSOutput input)
{
    GBufferOutput output;
    
    // UV Transform
    float2 uv = input.uv_and_pad.xy;
    
    float2 transformedUV = mul(float4(uv, 0.0f, 1.0f), uvTransform).xy;

    // ------- Albedo -------
    float4 baseColor = baseColorMap.Sample(samLinear, transformedUV);

    output.baseColor_Roughness = float4(baseColor.rgb, roughness);

    // ------- Normal -------
    float3 normalWS = normalize(input.normalWS.xyz);
    float3 packedNormal = normalWS * 0.5f + 0.5f;
    output.normal_Metal = float4(packedNormal, metal);

    // ------- World Pos -------
    output.worldPos_Padding = input.worldPosWS;
    
    return output;
}