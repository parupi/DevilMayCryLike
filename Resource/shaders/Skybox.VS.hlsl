#include "Skybox.hlsli"

struct TransformationMatrix
{
    float4x4 ViewProjection;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float3 texcoord : TEXCOORD0;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.ViewProjection).xyww;
    output.texcoord = input.texcoord.xyz;
    
    return output;
}