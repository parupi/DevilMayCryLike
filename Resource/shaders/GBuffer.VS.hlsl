#include "GBuffer.hlsli"
#include "TransformCommon.hlsli"

VSOutput main(VertexInput input)
{
    VSOutput output;

    // 安全のため全フィールドを初期化
    output.positionCS = float4(0, 0, 0, 0);
    output.normalWS = float4(0, 0, 0, 0);
    output.uv_and_pad = float4(0, 0, 0, 0);
    output.worldPosWS = float4(0, 0, 0, 0);
    
    // world position (WS)
    float4 worldPos = mul(input.position, World);
    output.worldPosWS = worldPos;

    // clip space position
    output.positionCS = mul(input.position, WVP);

    // normal (world space)
    float3 worldNormal = normalize(mul(float4(input.normal.xyz, 0.0f), WorldInverseTranspose).xyz);
    output.normalWS = float4(worldNormal, 1.0f);

    // uv
    output.uv_and_pad = float4(input.uv, 0.0f, 0.0f);

    return output;
}

