// Lighting.hlsli
struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Sampler
SamplerState sampler_Linear : register(s0);

// GBuffer textures
Texture2D<float4> gAlbedo : register(t0);
Texture2D<float4> gNormal : register(t1);
Texture2D<float4> gWorldPos : register(t2);
Texture2D<float4> gMaterial : register(t3);

// Directional light CB (例: register b1)
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

// Camera CB -> **register(b2)**
cbuffer CameraCB : register(b2)
{
    float4 CameraPosWS; // xyz = camera world position, w = padding
}

// Light count
cbuffer LightCountCB : register(b3)
{
    uint LightCount;
};

static const float3 BACKBUFFER_CLEAR_COLOR = float3(0.6f, 0.5f, 0.1f);

// Normal decode
float3 DecodeNormal(float3 packed)
{
    return normalize(packed * 2.0f - 1.0f);
}

// ShadowMap
Texture2D<float> gShadow : register(t5);

SamplerComparisonState sampler_Shadow : register(s1);

// Light VP
cbuffer LightMatrixCB : register(b4)
{
    float4x4 LightViewProj;
}

float CalcShadow(float3 worldPos)
{
    float4 shadowPos = mul(float4(worldPos, 1.0), LightViewProj);

    shadowPos.xyz /= shadowPos.w;

    float2 uv;
    uv.x = shadowPos.x * 0.5 + 0.5;
    uv.y = shadowPos.y * 0.5 + 0.5;

    float depth = shadowPos.z - 0.001f;
    
    // 範囲外チェック
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || depth < 0 || depth > 1)
        return 1.0;
    
    float shadow = (depth <= gShadow.Sample(sampler_Linear, uv)) ? 1.0 : 0.0;
    return shadow;
}