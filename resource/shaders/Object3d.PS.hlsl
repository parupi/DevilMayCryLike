#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct Camera
{
    float3 worldPosition;
};
ConstantBuffer<Camera> gCamera : register(b1);

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    int enabled; //!< DirectionalLightの有効フラグ
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    int enabled; //!< PointLightの有効フラグ
};


struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    int enabled; //!< SpotLightの有効フラグ
};

ConstantBuffer<DirectionalLight> gDirLight1 : register(b2);
ConstantBuffer<DirectionalLight> gDirLight2 : register(b3);
ConstantBuffer<DirectionalLight> gDirLight3 : register(b4);
ConstantBuffer<PointLight> gPointLight1 : register(b5);
ConstantBuffer<PointLight> gPointLight2 : register(b6);
ConstantBuffer<PointLight> gPointLight3 : register(b7);
ConstantBuffer<SpotLight> gSpotLight1 : register(b8);
ConstantBuffer<SpotLight> gSpotLight2 : register(b9);
ConstantBuffer<SpotLight> gSpotLight3 : register(b10);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    PixelShaderOutput output;
    
    if (textureColor.a == 0.0f)
    {
        discard;
    }
    if (output.color.a == 0.0f)
    {
        discard;
    }

    if (gMaterial.enableLighting != 0)
    {
        float3 finalDiffuse = float3(0.0f, 0.0f, 0.0f);
        float3 finalSpecular = float3(0.0f, 0.0f, 0.0f);
        
        float3 normal = normalize(input.normal);
        
        if (gDirLight1.enabled != 0)
        {
            // DirectionalLightの計算
            float NdotL = dot(normal, normalize(-gDirLight1.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-gDirLight1.direction + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gMaterial.color.rgb * textureColor.rgb * gDirLight1.color.rgb * cos * gDirLight1.intensity;
            finalSpecular += gDirLight1.color.rgb * gDirLight1.intensity * specularPow;
        }
        if (gDirLight2.enabled != 0)
        {
            // DirectionalLightの計算
            float NdotL = dot(normal, normalize(-gDirLight2.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-gDirLight2.direction + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gMaterial.color.rgb * textureColor.rgb * gDirLight2.color.rgb * cos * gDirLight2.intensity;
            finalSpecular += gDirLight2.color.rgb * gDirLight2.intensity * specularPow;
        }
        if (gDirLight3.enabled != 0)
        {
            // DirectionalLightの計算
            float NdotL = dot(normal, normalize(-gDirLight3.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-gDirLight3.direction + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gMaterial.color.rgb * textureColor.rgb * gDirLight3.color.rgb * cos * gDirLight3.intensity;
            finalSpecular += gDirLight3.color.rgb * gDirLight3.intensity * specularPow;
        }

        if (gPointLight1.enabled != 0)
        {
            // PointLightの計算
            float3 pointLightDirection = normalize(input.worldPosition - gPointLight1.position);
            float distance = length(gPointLight1.position - input.worldPosition);
            float factor = pow(saturate(-distance / gPointLight1.radius + 1.0), gPointLight1.decay);
            float NdotL = dot(normal, -pointLightDirection);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-pointLightDirection + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gPointLight1.color.rgb * gPointLight1.intensity * factor * gMaterial.color.rgb * textureColor.rgb * cos;
            finalSpecular += gPointLight1.color.rgb * gPointLight1.intensity * factor * specularPow;
        }
        if (gPointLight2.enabled != 0)
        {
            // PointLightの計算
            float3 pointLightDirection = normalize(input.worldPosition - gPointLight2.position);
            float distance = length(gPointLight2.position - input.worldPosition);
            float factor = pow(saturate(-distance / gPointLight2.radius + 1.0), gPointLight2.decay);
            float NdotL = dot(normal, -pointLightDirection);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-pointLightDirection + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gPointLight2.color.rgb * gPointLight2.intensity * factor * gMaterial.color.rgb * textureColor.rgb * cos;
            finalSpecular += gPointLight2.color.rgb * gPointLight2.intensity * factor * specularPow;
        }
        if (gPointLight3.enabled != 0)
        {
            // PointLightの計算
            float3 pointLightDirection = normalize(input.worldPosition - gPointLight3.position);
            float distance = length(gPointLight3.position - input.worldPosition);
            float factor = pow(saturate(-distance / gPointLight3.radius + 1.0), gPointLight3.decay);
            float NdotL = dot(normal, -pointLightDirection);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-pointLightDirection + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gPointLight3.color.rgb * gPointLight3.intensity * factor * gMaterial.color.rgb * textureColor.rgb * cos;
            finalSpecular += gPointLight3.color.rgb * gPointLight3.intensity * factor * specularPow;
        }

        if (gSpotLight1.enabled != 0)
        {
            // SpotLightの計算
            float3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight1.position);
            float cosAngle = dot(spotLightDirectionOnSurface, gSpotLight1.direction);
            float cosFalloffStart = gSpotLight1.cosAngle * 2.0f;
            float falloffFactor = pow(saturate((cosAngle - gSpotLight1.cosAngle) / (cosFalloffStart - gSpotLight1.cosAngle)), gSpotLight1.decay);
            float NdotL = dot(normal, -spotLightDirectionOnSurface);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-spotLightDirectionOnSurface + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gSpotLight1.color.rgb * gSpotLight1.intensity * falloffFactor * gMaterial.color.rgb * textureColor.rgb * cos;
            finalSpecular += gSpotLight1.color.rgb * gSpotLight1.intensity * falloffFactor * specularPow;
        }
        if (gSpotLight2.enabled != 0)
        {
            // SpotLightの計算
            float3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight2.position);
            float cosAngle = dot(spotLightDirectionOnSurface, gSpotLight2.direction);
            float cosFalloffStart = gSpotLight2.cosAngle * 2.0f;
            float falloffFactor = pow(saturate((cosAngle - gSpotLight2.cosAngle) / (cosFalloffStart - gSpotLight2.cosAngle)), gSpotLight2.decay);
            float NdotL = dot(normal, -spotLightDirectionOnSurface);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-spotLightDirectionOnSurface + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gSpotLight2.color.rgb * gSpotLight2.intensity * falloffFactor * gMaterial.color.rgb * textureColor.rgb * cos;
            finalSpecular += gSpotLight2.color.rgb * gSpotLight2.intensity * falloffFactor * specularPow;
        }
        if (gSpotLight3.enabled != 0)
        {
            // SpotLightの計算
            float3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight3.position);
            float cosAngle = dot(spotLightDirectionOnSurface, gSpotLight3.direction);
            float cosFalloffStart = gSpotLight3.cosAngle * 2.0f;
            float falloffFactor = pow(saturate((cosAngle - gSpotLight3.cosAngle) / (cosFalloffStart - gSpotLight3.cosAngle)), gSpotLight3.decay);
            float NdotL = dot(normal, -spotLightDirectionOnSurface);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 halfVector = normalize(-spotLightDirectionOnSurface + toEye);
            float NdotH = dot(normal, halfVector);
            float specularPow = pow(saturate(NdotH), gMaterial.shininess);

            // 拡散反射と鏡面反射を加算
            finalDiffuse += gSpotLight3.color.rgb * gSpotLight3.intensity * falloffFactor * gMaterial.color.rgb * textureColor.rgb * cos;
            finalSpecular += gSpotLight3.color.rgb * gSpotLight3.intensity * falloffFactor * specularPow;
        }
        
        // 拡散反射 + 鏡面反射
        output.color.rgb = finalDiffuse + finalSpecular;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }

    return output;
}