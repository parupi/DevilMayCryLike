Texture2D tex0 : register(t0);
SamplerState smp : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = tex0.Sample(smp, input.uv);
    
    if (input.color.a == 0.0f)
    {
        discard;
    }
    
    if (texColor.a == 0.0f)
    {
        discard;
    }
    
    return texColor * input.color;
}
