// GBufferVS.hlsl
#include "GBuffer.hlsli"
#include "TransformCommon.hlsli"

VSOutput main(VertexInput input)
{
    VSOutput output;

    float4 worldPos = mul(float4(input.position.xyz, 1.0f), World);
    float4 viewPos = mul(worldPos, View);
    output.positionCS = mul(viewPos, Proj);

    // normal → view space
    float3 worldNormal = normalize(mul(float4(input.normal, 0), World).xyz);
    float3 viewNormal = normalize(mul(float4(worldNormal, 0), View).xyz);
    output.normalVS = viewNormal;

    output.uv = input.uv;
    return output;
}
