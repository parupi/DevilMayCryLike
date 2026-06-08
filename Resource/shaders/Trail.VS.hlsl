cbuffer TrailCB : register(b0)
{
    float4x4 viewProj;
    float4   tintColor;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color    : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color    : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), viewProj);
    output.texcoord = input.texcoord;
    output.color    = input.color * tintColor;
    return output;
}
