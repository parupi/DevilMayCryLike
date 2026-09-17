#include "Particle.hlsli"

struct ParticleForGPU
{
    float4x4 WVP;
    float4x4 World;
    float4 color;
    // スプライトシートのコマ切り出し。xy = UVオフセット / zw = UVスケール。
    // コマ分割なしのときは (0,0,1,1) が入るので texcoord はそのまま通る
    float4 uvOffsetScale;
    // x = 粒ごとの乱数 / y = 寿命の進み具合
    float4 misc;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

StructuredBuffer<ParticleForGPU> gParticle : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gParticle[instanceId].WVP);

    float4 uv = gParticle[instanceId].uvOffsetScale;
    output.texcoord = input.texcoord * uv.zw + uv.xy;
    output.meshTexcoord = input.texcoord;

    output.color = gParticle[instanceId].color;
    output.worldPos = mul(input.position, gParticle[instanceId].World).xyz;
    output.misc = gParticle[instanceId].misc;

    return output;
}
