#include "Lighting.hlsli"

struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.pos = float4(input.pos, 1.0f);
    output.uv = input.uv;
    return output;
}