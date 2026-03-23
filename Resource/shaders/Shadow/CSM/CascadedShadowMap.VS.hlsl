cbuffer LightVP : register(b1)
{
    float4x4 lightViewProj;
};

cbuffer Object : register(b0)
{
    float4x4 world;
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

    float4 worldPos = mul(world, float4(input.pos, 1));
    o.pos = mul(lightViewProj, worldPos);

    return o;
    
    //VSOutput o;

    //float4 worldPos = mul(float4(input.pos, 1), world);

    //o.pos = worldPos; // LightVPを無視
    //o.pos = float4(input.pos.xy, 0, 1);

    //return o;
}