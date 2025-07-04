#include "Skybox.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float3 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP).xyww;
    output.texcoord = input.texcoord.xyz;
    
    return output;
}