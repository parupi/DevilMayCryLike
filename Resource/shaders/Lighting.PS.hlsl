#include "Lighting.hlsli"

SamplerState sampler_Linear : register(s0);

Texture2D<float4> gAlbedo : register(t0);
Texture2D<float4> gNormal : register(t1);
Texture2D<float4> gWorldPos : register(t2);
Texture2D<float4> gMaterial : register(t3);

cbuffer DirectionalLightCB : register(b1)
{
    float4 lightColor;
    float4 lightDir;
    float intensity;
    int enabled;
    float2 padding;
};

static const float3 ambientColor = float3(0.06f, 0.06f, 0.06f);

float4 main(PSInput input) : SV_Target
{
    float2 uv = input.uv;
    
    float4 albedo = gAlbedo.Sample(sampler_Linear, uv);
    float4 nrmS = gNormal.Sample(sampler_Linear, uv);
    float4 posS = gWorldPos.Sample(sampler_Linear, uv);
    float4 mat = gMaterial.Sample(sampler_Linear, uv);

     // reconstruct normal (assume stored in xyz)
    float3 N = normalize(nrmS.xyz);

    // directional light vector (assume lightDir is direction TO light). For Lambert, use L = normalize(lightDir)
    float3 L = normalize(lightDir.xyz);

    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo.rgb * lightColor.rgb * NdotL * intensity;

    // simple specular (Blinn-Phong) using view direction approx from worldpos towards camera at origin (if camera not at origin, pass camera pos later)
    float3 V = normalize(-posS.xyz); // NOTE: assumes camera at origin; for accurate result pass camera pos uniform
    float3 H = normalize(L + V);
    float specPower = lerp(16.0f, 64.0f, mat.r); // example use of material.r as gloss
    float spec = pow(saturate(dot(N, H)), specPower);
    float3 specular = spec * lightColor.rgb * 0.2f;

    float3 color = ambientColor + diffuse + specular;

    // apply albedo and material emission (material.a used as emissive)
    color = color * albedo.rgb + mat.a * albedo.rgb * 0.0f;

    return float4(color, 1.0f);
}