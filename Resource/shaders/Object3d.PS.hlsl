#include "Object3d.hlsli"
// -----------------------------
// 共通定義（Lighting.hlsli と同じもの）
// -----------------------------
SamplerState gSampler : register(s0);
Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);

// Material (object specific)
struct Material
{
    float4 color;
    int enableLighting;
    float environmentIntensity;
    float2 padding; // 16-byte alignment
    float4x4 uvTransform;
    float shininess;
    float3 padding2;
};
ConstantBuffer<Material> gMaterial : register(b0);

// Camera -> register(b2) に合わせる
struct Camera
{
    float3 worldPosition;
    float pad;
};
ConstantBuffer<Camera> gCamera : register(b2);

// LightData (same layout as Lighting.hlsli)
struct LightData
{
    uint type; // 0:Directional 1:Point 2:Spot
    uint enabled;
    float intensity;
    float decay;

    float4 color;

    float3 position;
    float radius;

    float3 direction;
    float cosAngle;

    float distance;
    float3 padding;
};

StructuredBuffer<LightData> Lights : register(t4);

cbuffer LightCountCB : register(b3)
{
    uint LightCount;
};

// -----------------------------
// 入力／出力 (頂点シェーダ側から受け取る構造体と合わせる)
// -----------------------------
struct PSInput
{
    float4 pos : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texcoord : TEXCOORD2;
};

// -----------------------------
// 共通ユーティリティ関数
// -----------------------------
static const float3 BACKBUFFER_CLEAR_COLOR = float3(0.6f, 0.5f, 0.1f);

float3 DecodeNormal(float3 packed)
{
    return normalize(packed * 2.0f - 1.0f);
}

// Directional light
float3 CalcDirectionalLight(LightData light, float3 N, float3 V, float3 albedo, float shininess)
{
    float3 L = normalize(-light.direction);
    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * light.color.rgb * NdotL * light.intensity;

    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), max(shininess, 1e-4));
    float3 specular = spec * light.color.rgb * 0.2 * (1.0 - 0.0); // metal term not in Material here

    return diffuse + specular;
}

// Point light
float3 CalcPointLight(LightData light, float3 P, float3 N, float3 V, float3 albedo, float shininess)
{
    float3 Lvec = light.position - P;
    float dist = length(Lvec);
    if (dist > light.radius)
        return 0;

    float3 L = Lvec / dist;
    float atten = 1.0 / (1.0 + light.decay * dist * dist);

    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * light.color.rgb * NdotL * light.intensity * atten;

    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), max(shininess, 1e-4));
    float3 specular = spec * light.color.rgb * atten * 0.2;

    return diffuse + specular;
}

// Spot light
float3 CalcSpotLight(LightData light, float3 P, float3 N, float3 V, float3 albedo, float shininess)
{
    float3 Lvec = light.position - P;
    float dist = length(Lvec);
    if (dist > light.distance)
        return 0;

    float3 L = Lvec / dist;
    float spot = dot(normalize(-light.direction), L);
    if (spot < light.cosAngle)
        return 0;

    float atten = 1.0 / (1.0 + light.decay * dist * dist);
    float spotFactor = smoothstep(light.cosAngle, 1.0, spot);

    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * light.color.rgb * NdotL * light.intensity * atten * spotFactor;

    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), max(shininess, 1e-4));
    float3 specular = spec * light.color.rgb * atten * spotFactor * 0.2;

    return diffuse + specular;
}

// -----------------------------
// メイン
// -----------------------------
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // テクスチャUV変換
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (textureColor.a == 0.0f)
        discard;

    // 基本色
    float3 albedo = gMaterial.color.rgb * textureColor.rgb;

    // 法線・視線
    float3 N = normalize(input.normal);
    float3 V = normalize(gCamera.worldPosition - input.worldPosition);

    float3 finalDiffuse = float3(0.0, 0.0, 0.0);
    float3 finalSpecular = float3(0.0, 0.0, 0.0);

    if (gMaterial.enableLighting != 0 && LightCount > 0)
    {
        // すべてのライトに対してループ（Lights は t4 にバインドされている前提）
        for (uint i = 0; i < LightCount; ++i)
        {
            LightData light = Lights[i];
            if (light.enabled == 0)
                continue;

            if (light.type == 0)
            {
                float3 add = CalcDirectionalLight(light, N, V, albedo, gMaterial.shininess);
                finalDiffuse += add; // Calc returns diffuse+specular mixture for directional
            }
            else if (light.type == 1)
            {
                finalDiffuse += CalcPointLight(light, input.worldPosition, N, V, albedo, gMaterial.shininess);
            }
            else if (light.type == 2)
            {
                finalDiffuse += CalcSpotLight(light, input.worldPosition, N, V, albedo, gMaterial.shininess);
            }
        }
    }
    else
    {
        finalDiffuse = albedo;
    }

    // 環境マップ
    float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
    float3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
    float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);

    float3 finalColor = finalDiffuse + finalSpecular + environmentColor.rgb * gMaterial.environmentIntensity;

    output.color = float4(finalColor, textureColor.a);
    return output;
}
