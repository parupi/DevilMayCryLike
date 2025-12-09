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
    float4 normalWS : TEXCOORD0; // xyzのみ使用、wは捨て
    float4 uv_and_pad : TEXCOORD1; // xy=uv, zw=padding
    float4 worldPosWS : TEXCOORD2; // ワールド座標
};

struct GBufferOutput
{
    float4 baseColor_Roughness : SV_Target0; // rgb = baseColor , a = roughness
    float4 normal_Metal : SV_Target1; // rgb = packed normal (0..1), a = metal
    float4 worldPos_Padding : SV_Target2; // xyz = world position (WS), w = unused/padding
    float depth : SV_Depth;
};

#endif
