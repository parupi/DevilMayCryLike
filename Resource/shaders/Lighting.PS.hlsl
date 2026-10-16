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
    // 内側の角度（padding.z）があればそこまで最も明るく、無ければ従来どおり中心だけが最も明るい
    float innerCos = light.padding.z > light.cosAngle ? light.padding.z : 1.0;
    float spotFactor = smoothstep(light.cosAngle, innerCos, spot);

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
        // 何も描かれていない所でも、松明の光が空気中で散る分は見える
        float3 background = BackgroundColor;
        if (VolumetricStrength > 0.0f || SpotVolumetricStrength > 0.0f)
        {
            float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
            float4 farPoint = mul(float4(ndc, 1.0f, 1.0f), InvViewProj);
            float3 dir = normalize(farPoint.xyz / farPoint.w - CameraPosWS.xyz);
            float3 scatter = CalcVolumetricLight(CameraPosWS.xyz, dir, VolumetricMaxDistance)
                + CalcSpotVolumetric(CameraPosWS.xyz, dir, VolumetricMaxDistance, input.pos.xy);
            // デバッグ表示: 光の筋だけ
            if (DebugView > 0.5f && DebugView < 1.5f)
                return float4(scatter, 1.0f);
            background += scatter;
        }
        // デバッグ表示: 埃だけを見るときは背景を黒に
        if (DebugView > 0.5f)
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        return float4(background, 1.0f);
    }

    float4 albedoRough = gAlbedo.Sample(sampler_Linear, uv);
    float3 albedo = albedoRough.rgb;
    float roughness = albedoRough.a;
    
    float4 normalMetal = gNormal.Sample(sampler_Linear, uv);
    float3 N = DecodeNormal(normalMetal.rgb);
    float metal = normalMetal.a;

    float3 V = normalize(CameraPosWS.xyz - P);

    // SSAO。環境光にはしっかり、直接光には弱く掛ける（暗い場所で直接光まで削ると黒く潰れる）
    float ao = gAO.Load(int3(input.pos.xy, 0));
    float aoAmbient = lerp(1.0f, ao, SsaoAmbientStrength);
    float aoDirect = lerp(1.0f, ao, SsaoDirectStrength);

    float3 ambient = AmbientColor * lerp(float3(1.0f, 1.0f, 1.0f), albedo, AmbientAlbedoMix);
    float3 direct = float3(0.0f, 0.0f, 0.0f);

    float shadow = CalcShadow(P);

    for (uint i = 0; i < LightCount; i++)
    {
        LightData light = Lights[i];
        if (!light.enabled) continue;

        if (light.type == 0)
            direct += CalcDirectionalLight(light, N, V, albedo) * shadow * DirectionalScale;

        else if (light.type == 1)
            direct += CalcPointLight(light, P, N, V, albedo);

        else if (light.type == 2)
            direct += CalcSpotLight(light, P, N, V, albedo);
    }

    float3 finalColor = ambient * aoAmbient + direct * aoDirect;

    // キャラ（環境オブジェクト以外）だけに足す光。暗い洞窟でもプレイヤーや敵のシルエットが読めるようにする
    if (posS.w < 1.5f)
    {
        finalColor += CharacterAmbient * albedo * aoAmbient;
        float rim = pow(1.0f - saturate(dot(N, V)), max(RimPower, 0.1f));
        finalColor += RimColor * rim;
    }

    // 高さによる暗化。壁の上ほど暗くして、壁の上端を闇に溶かす（プレイヤーや敵には掛けない）
    if (posS.w > 1.5f && DarkStrength > 0.0f)
    {
        float heightFactor = saturate((P.y - DarkStartHeight) / max(DarkEndHeight - DarkStartHeight, 0.001f));
        finalColor *= 1.0f - heightFactor * DarkStrength;
    }

    // 距離フォグ。遠くの壁ほど暗がりに沈めて奥行きを出す（松明の炎や攻撃予兆はフォワードで描くので掛からない）
    finalColor = lerp(finalColor, DistanceFogColor, CalcDistanceFog(CameraPosWS.xyz, P));

    // 高さフォグ。壁の上端を霧の中へ消す（背景色に近い色にすると、空との境目も消える）
    finalColor = lerp(finalColor, HeightFogColor, CalcHeightFog(CameraPosWS.xyz, P));

    // 空気中で散る松明の光（物の手前にある分だけ。霧の後に足すので霧に隠れない）
    float viewDistance = length(P - CameraPosWS.xyz);
    float3 viewDir = (P - CameraPosWS.xyz) / max(viewDistance, 1e-4f);
    float3 scatter = CalcVolumetricLight(CameraPosWS.xyz, viewDir, viewDistance)
        // 天井から差すスポットライトの光の筋（物に当たった所で止まる）
        + CalcSpotVolumetric(CameraPosWS.xyz, viewDir, viewDistance, input.pos.xy);
    finalColor += scatter;

    // デバッグ表示: 1=光の筋だけ / 2・3=埃だけを見るので物は黒に
    if (DebugView > 0.5f)
        finalColor = DebugView < 1.5f ? scatter : float3(0.0f, 0.0f, 0.0f);

    return float4(finalColor, 1.0f);
}