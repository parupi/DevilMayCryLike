#include "Object3d.hlsli"

// ライトの最大数
#define MAX_DIR_LIGHTS  3
#define MAX_POINT_LIGHTS 3
#define MAX_SPOT_LIGHTS  3

struct Material
{
    float4 color;
    int enableLighting;
    float environmentIntensity;
    float2 padding;  // 16バイトアラインメント
    float4x4 uvTransform;
    float shininess;
    float3 padding2;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct Camera
{
    float3 worldPosition;
};
ConstantBuffer<Camera> gCamera : register(b1);

// Directional Light
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    int enabled;
};
cbuffer DirectionalLights : register(b2)
{
    DirectionalLight gDirLights[MAX_DIR_LIGHTS];
}

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    int enabled; //!< PointLightの有効フラグ
};
cbuffer PointLights : register(b3)
{
    PointLight gPointLights[MAX_POINT_LIGHTS];
}


// Spot Light
struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    int enabled;
};
cbuffer SpotLights : register(b4)
{
    SpotLight gSpotLights[MAX_SPOT_LIGHTS];
}

// 環境マップ
TextureCube<float32_t4> gEnvironmentTexture : register(t1);

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (textureColor.a == 0.0f)
        discard;

    float3 finalDiffuse = 0;
    float3 finalSpecular = 0;
    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(gCamera.worldPosition - input.worldPosition);

    if (gMaterial.enableLighting != 0)
    {
        // Directional Lights
        [unroll]
        for (int i = 0; i < MAX_DIR_LIGHTS; ++i)
        {
            if (gDirLights[i].enabled == 0)
                continue;

            float3 lightDir = normalize(-gDirLights[i].direction);
            float NdotL = dot(normal, lightDir);
            float diff = pow(NdotL * 0.5f + 0.5f, 2.0f);

            float3 halfVector = normalize(lightDir + viewDir);
            float NdotH = dot(normal, halfVector);
            float spec = pow(saturate(NdotH), gMaterial.shininess);

            finalDiffuse += gMaterial.color.rgb * textureColor.rgb * gDirLights[i].color.rgb * diff * gDirLights[i].intensity;
            finalSpecular += gDirLights[i].color.rgb * gDirLights[i].intensity * spec;
        }

        // Point Lights
        [unroll]
        for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
        {
            if (gPointLights[i].enabled == 0)
                continue;

            float3 lightVec = input.worldPosition - gPointLights[i].position;
            float3 lightDir = normalize(lightVec);
            float distance = length(lightVec);
            float factor = pow(saturate(-distance / gPointLights[i].radius + 1.0f), gPointLights[i].decay);

            float NdotL = dot(normal, -lightDir);
            float diff = pow(NdotL * 0.5f + 0.5f, 2.0f);

            float3 halfVector = normalize(-lightDir + viewDir);
            float NdotH = dot(normal, halfVector);
            float spec = pow(saturate(NdotH), gMaterial.shininess);

            finalDiffuse += gPointLights[i].color.rgb * gPointLights[i].intensity * factor * gMaterial.color.rgb * textureColor.rgb * diff;
            finalSpecular += gPointLights[i].color.rgb * gPointLights[i].intensity * factor * spec;
        }

        // Spot Lights
        [unroll]
        for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
        {
            if (gSpotLights[i].enabled == 0)
                continue;

            float3 toSurface = normalize(input.worldPosition - gSpotLights[i].position);
            float cosAngle = dot(toSurface, gSpotLights[i].direction);
            float cosFalloffStart = gSpotLights[i].cosAngle * 2.0f;
            float falloffFactor = pow(saturate((cosAngle - gSpotLights[i].cosAngle) / (cosFalloffStart - gSpotLights[i].cosAngle)), gSpotLights[i].decay);

            float NdotL = dot(normal, -toSurface);
            float diff = pow(NdotL * 0.5f + 0.5f, 2.0f);

            float3 halfVector = normalize(-toSurface + viewDir);
            float NdotH = dot(normal, halfVector);
            float spec = pow(saturate(NdotH), gMaterial.shininess);

            finalDiffuse += gSpotLights[i].color.rgb * gSpotLights[i].intensity * falloffFactor * gMaterial.color.rgb * textureColor.rgb * diff;
            finalSpecular += gSpotLights[i].color.rgb * gSpotLights[i].intensity * falloffFactor * spec;
        }
        
        
    }
    else
    {
        finalDiffuse = gMaterial.color.rgb * textureColor.rgb;
    }

    // 環境マップの計算
    float32_t3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
    float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
    float32_t4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);

    // 環境マップを反映
    float3 finalColor = finalDiffuse + finalSpecular + environmentColor.rgb * gMaterial.environmentIntensity;

    output.color = float4(finalColor, textureColor.a);
    return output;
}