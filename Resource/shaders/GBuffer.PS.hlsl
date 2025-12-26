#include "GBuffer.hlsli"

Texture2D baseColorMap : register(t0);
SamplerState samLinear : register(s0);

cbuffer MaterialParam : register(b0)
{
    float roughness;
    float metal;
    float2 padding;
};

float Linear01Depth(float4 posCS)
{
    return 1.0f - (posCS.z / posCS.w);
}

GBufferOutput main(VSOutput input)
{
    GBufferOutput output;

    // ------- Albedo -------
    float4 baseColor = baseColorMap.Sample(samLinear, input.uv_and_pad.xy);

    output.baseColor_Roughness = float4(baseColor.rgb, roughness);

    // ------- Normal -------
    float3 normalWS = normalize(input.normalWS.xyz);
    float3 packedNormal = normalWS * 0.5f + 0.5f;
    output.normal_Metal = float4(packedNormal, metal);

    // ------- World Pos -------
    output.worldPos_Padding = input.worldPosWS;
    
    // ------- Depth -------
    output.depth = Linear01Depth(input.positionCS);
    

    return output;
}

//float4 main(VSOutput input) : SV_Target0
//{
//    // Debug PSO: 出力 color = depth (z/w) as grey
//    float depth = input.positionCS.z / input.positionCS.w;
//    return float4(depth, depth, depth, 1);
//}