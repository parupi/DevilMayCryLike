// DeferredLightingPS.hlsl
#include "GBuffer.hlsli"

Texture2D GBuffer0 : register(t0); // Base/Roughness
Texture2D GBuffer1 : register(t1); // Normal/Metal
Texture2D GBuffer2 : register(t2); // Depth
SamplerState samLinear : register(s0);

float4 main(float4 posCS : SV_POSITION, float2 uv : TEXCOORD0) : SV_Target
{
    float4 baseR = GBuffer0.Sample(samLinear, uv);
    float4 normM = GBuffer1.Sample(samLinear, uv);

    float3 baseColor = baseR.rgb;
    float roughness = baseR.a;
    float3 normalVS = normalize(normM.rgb * 2.0f - 1.0f);
    float metal = normM.a;

    float depth = GBuffer2.Sample(samLinear, uv).r;

    // ここで light calc
    // (今回は skeleton なので戻りだけ)
    return float4(baseColor, 1);
}
