Texture2D<float4> gTexture : register(t0);
SamplerState      gSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color    : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float4 outColor = texColor * input.color;
    if (outColor.a < 0.01f)
        discard;
    return outColor;
}
