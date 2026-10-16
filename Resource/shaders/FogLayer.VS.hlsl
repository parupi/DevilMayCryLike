#include "FogLayer.hlsli"

// 頂点バッファは使わず、頂点番号から水平な板の四隅を組む（2枚の三角形）
static const float2 kCorners[6] =
{
    float2(-1.0f, -1.0f), float2(1.0f, -1.0f), float2(1.0f, 1.0f),
    float2(-1.0f, -1.0f), float2(1.0f, 1.0f), float2(-1.0f, 1.0f),
};

FogVSOutput main(uint vertexId : SV_VertexID)
{
    FogVSOutput output;
    float2 c = kCorners[vertexId];
    float3 world = gCenter + float3(c.x * gHalfSize, 0.0f, c.y * gHalfSize);
    output.position = mul(float4(world, 1.0f), gViewProj);
    output.worldPos = world;
    output.local = c;
    return output;
}
