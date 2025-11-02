// GBufferCommon.hlsli

#ifndef GBUFFER_COMMON
#define GBUFFER_COMMON

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 positionCS : SV_POSITION;
    float3 normalVS : NORMAL0;
    float2 uv : TEXCOORD0;
};

struct GBufferOutput
{
    float4 baseColor_Roughness : SV_Target0; // rgb = baseColor , a = roughness
    float4 normal_Metal : SV_Target1; // rgb = normal(view space), a = metal
    float4 depthDummy : SV_Target2; // depth 値を書いておく(Linear depthでもOK)
};

#endif