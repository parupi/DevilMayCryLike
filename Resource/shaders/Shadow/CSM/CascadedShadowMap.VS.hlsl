cbuffer ObjectCB : register(b0)
{
    float4x4 World;
};

cbuffer LightCB : register(b1)
{
    float4x4 LightViewProj;
};

struct VSInput
{
    float3 pos : POSITION;
};

struct VSOutput
{
    float4 svpos : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput o;
    float4 worldPos = mul(float4(input.pos, 1.0f), World);
    o.svpos = mul(worldPos, LightViewProj);
    return o;
}
