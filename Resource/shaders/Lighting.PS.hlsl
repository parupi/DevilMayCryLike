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

    // w はフラグなので補間されないよう Load で読む（0=何も無い / 1=通常 / 2=環境オブジェクト）
    float4 posS = gWorldPos.Load(int3(input.pos.xy, 0));
    float3 P = posS.xyz;

    if (posS.w < 0.5f)
    {
        return float4(BackgroundColor, 1.0f);
    }

    float4 albedoRough = gAlbedo.Sample(sampler_Linear, uv);
    float3 albedo = albedoRough.rgb;
    float roughness = albedoRough.a;
    
    float4 normalMetal = gNormal.Sample(sampler_Linear, uv);
    float3 N = DecodeNormal(normalMetal.rgb);
    float metal = normalMetal.a;

    float3 V = normalize(CameraPosWS.xyz - P);

    float3 finalColor = AmbientColor * lerp(float3(1.0f, 1.0f, 1.0f), albedo, AmbientAlbedoMix);

    float shadow = CalcShadow(P);

    for (uint i = 0; i < LightCount; i++)
    {
        LightData light = Lights[i];
        if (!light.enabled) continue;

        if (light.type == 0)
            finalColor += CalcDirectionalLight(light, N, V, albedo) * shadow * DirectionalScale;

        else if (light.type == 1)
            finalColor += CalcPointLight(light, P, N, V, albedo);

        else if (light.type == 2)
            finalColor += CalcSpotLight(light, P, N, V, albedo);
    }

    // 高さによる暗化。壁の上ほど暗くして、壁の上端を闇に溶かす（プレイヤーや敵には掛けない）
    if (posS.w > 1.5f && DarkStrength > 0.0f)
    {
        float heightFactor = saturate((P.y - DarkStartHeight) / max(DarkEndHeight - DarkStartHeight, 0.001f));
        finalColor *= 1.0f - heightFactor * DarkStrength;
    }

    return float4(finalColor, 1.0f);
}