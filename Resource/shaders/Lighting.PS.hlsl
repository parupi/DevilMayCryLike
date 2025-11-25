#include "Lighting.hlsli"

static const float3 BACKBUFFER_CLEAR_COLOR = float3(0.6f, 0.5f, 0.1f);

float3 DecodeNormal(float3 packed)
{
    return normalize(packed * 2.0f - 1.0f);
}

// ---------------------------------------------
// メインパス
// ---------------------------------------------
float4 main(PSInput input) : SV_Target
{
    float2 uv = input.uv;

    // worldPos をサンプル（通常は Sample で OK）
    float4 posS = gWorldPos.Sample(sampler_Linear, uv);
    
    float3 worldPos = posS.xyz;

    // worldPos がほぼゼロなら "ジオメトリが無い" と判断してクリア色を返す
    if (length(worldPos) < 1e-4f)
    {
        return float4(BACKBUFFER_CLEAR_COLOR, 1.0f);
    }

    // GBuffer 読み出し
    float4 albedoRough = gAlbedo.Sample(sampler_Linear, uv);
    float3 albedo = albedoRough.rgb;
    float roughness = albedoRough.a;

    float4 normalMetal = gNormal.Sample(sampler_Linear, uv);
    float3 N = DecodeNormal(normalMetal.rgb);
    float metal = normalMetal.a;

    float3 L = normalize(-lightDir.xyz);
    float3 V = normalize(CameraPosWS.xyz - worldPos);

    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * lightColor.rgb * NdotL * intensity;

    float3 H = normalize(L + V);
    float specPower = lerp(16.0f, 64.0f, 1.0f - roughness);
    float spec = pow(saturate(dot(N, H)), specPower);
    float3 specular = spec * lightColor.rgb * (1.0f - metal) * 0.2f;

    float3 ambient = float3(0.06f, 0.06f, 0.06f);

    float3 color = ambient + diffuse + specular;
    color = color * albedo;

    return float4(color, 1.0f);
}