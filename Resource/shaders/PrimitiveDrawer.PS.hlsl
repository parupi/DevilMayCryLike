SamplerState smp : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    if (input.color.a == 0.0f)
    {
        discard;
    }
    
    return input.color;
}
