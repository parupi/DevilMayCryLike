// GBufferCommon.hlsli

#ifndef GBUFFER_COMMON
#define GBUFFER_COMMON

struct VertexInput
{
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normalVS : NORMAL0;
};

struct GBufferOutput
{
    float4 baseColor_Roughness : SV_Target0; // rgb = baseColor , a = roughness
    float4 normal_Metal : SV_Target1; // rgb = normal(view space), a = metal
    float4 depthDummy : SV_Target2; // depth 値を書いておく(Linear depthでもOK)
};

#endif