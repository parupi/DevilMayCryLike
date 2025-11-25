// Lighting.PS.hlsl
#include "Lighting.hlsli"

// -------------------------------
// 各ライトの計算
// -------------------------------

// Directional Light
float3 CalcDirectionalLight(LightData light, float3 N, float3 V, float3 albedo)
{
    float3 L = normalize(-light.direction);
    float NdotL = saturate(dot(N, L));

    float3 diffuse = albedo * light.color.rgb * NdotL * light.intensity;

    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), 32.0);
    float3 specular = spec * light.color.rgb * 0.2;

    return diffuse + specular;
}

// Point Light
float3 CalcPointLight(LightData light, float3 P, float3 N, float3 V, float3 albedo)
{
    float3 L = light.position - P;
    float dist = length(L);
    if (dist > light.radius)
        return 0;

    L /= dist;

    float atten = 1.0 / (1.0 + light.decay * dist * dist);

    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * light.color.rgb * NdotL * light.intensity * atten;

    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), 32.0);
    float3 specular = spec * light.color.rgb * atten * 0.2;

    return diffuse + specular;
}

// Spot Light
float3 CalcSpotLight(LightData light, float3 P, float3 N, float3 V, float3 albedo)
{
    float3 L = light.position - P;
    float dist = length(L);
    if (dist > light.distance)
        return 0;

    L /= dist;

    float spot = dot(normalize(-light.direction), L);
    if (spot < light.cosAngle)
        return 0;

    float atten = 1.0 / (1.0 + light.decay * dist * dist);
    float spotFactor = smoothstep(light.cosAngle, 1.0, spot);

    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * light.color.rgb * NdotL * light.intensity * atten * spotFactor;

    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), 32.0);
    float3 specular = spec * light.color.rgb * atten * spotFactor * 0.2;

    return diffuse + specular;
}

// ---------------------------------------------
// メインパス
// ---------------------------------------------
float4 main(PSInput input) : SV_Target
{
    float2 uv = input.uv;

    float4 posS = gWorldPos.Sample(sampler_Linear, uv);
    float3 P = posS.xyz;

    if (length(P) < 1e-4f)
    {
        return float4(BACKBUFFER_CLEAR_COLOR, 1.0f);
    }

    float4 albedoRough = gAlbedo.Sample(sampler_Linear, uv);
    float3 albedo = albedoRough.rgb;
    float roughness = albedoRough.a;

    float4 normalMetal = gNormal.Sample(sampler_Linear, uv);
    float3 N = DecodeNormal(normalMetal.rgb);
    float metal = normalMetal.a;

    float3 V = normalize(CameraPosWS.xyz - P);

    float3 finalColor = float3(0.06, 0.06, 0.06); // ambient

    for (uint i = 0; i < LightCount; i++)
    {
        LightData light = Lights[i];
        if (!light.enabled) continue;

        if (light.type == 0)
            finalColor += CalcDirectionalLight(light, N, V, albedo);

        else if (light.type == 1)
            finalColor += CalcPointLight(light, P, N, V, albedo);

        else if (light.type == 2)
            finalColor += CalcSpotLight(light, P, N, V, albedo);
    }

    return float4(finalColor, 1.0f);
}