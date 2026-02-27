#include "Fullscreen.hlsli"

Texture2D<float4> gLighting : register(t0);
SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    return gLighting.Sample(gSampler, input.texcoord);
}