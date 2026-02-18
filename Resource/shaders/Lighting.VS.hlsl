// Lighting.VS.hlsl
#include "Lighting.hlsli"

struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

PSInput main(VSInput input)
{
    PSInput output;
    // fullscreen triangle positions already provided in vertex buffer. pass through.
    output.pos = float4(input.pos, 1.0f);
    output.uv = input.uv;
    return output;
}
