cbuffer LightVP : register(b1)
{
    float4x4 lightViewProj;
};

cbuffer Object : register(b0)
{
    float4x4 WVP;
    float4x4 world;
    float4x4 WorldInverseTranspose;
};

struct VSInput
{
    float3 pos : POSITION;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput o;
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    o.pos = mul(worldPos, lightViewProj);
    return o;
}
