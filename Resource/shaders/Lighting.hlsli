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

// ShadowMap (CSM 3カスケード)
Texture2D<float> gShadow0 : register(t5);
Texture2D<float> gShadow1 : register(t6);
Texture2D<float> gShadow2 : register(t7);

SamplerComparisonState sampler_Shadow : register(s1);

// CSM 定数バッファ (C++ の CascadeLightingCB と同一レイアウト)
cbuffer LightMatrixCB : register(b4)
{
    float4x4 LightViewProj0;   // cascade 0
    float4x4 LightViewProj1;   // cascade 1
    float4x4 LightViewProj2;   // cascade 2
    float4   CascadeFar;       // xyz = 各カスケードの view-space far 深度
    float4x4 CameraView;       // view-space Z 計算用
}

// 1枚のシャドウマップへの PCF 3x3 サンプリング
float SampleShadowPCF(Texture2D<float> shadowMap, float4x4 lvp, float3 worldPos)
{
    float4 shadowPos = mul(float4(worldPos, 1.0), lvp);
    shadowPos.xyz /= shadowPos.w;

    float2 uv;
    uv.x = shadowPos.x * 0.5 + 0.5;
    uv.y = -shadowPos.y * 0.5 + 0.5;

    float depth = shadowPos.z - 0.002f;

    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || depth < 0 || depth > 1)
        return 1.0;

    float shadow = 0.0;
    float2 texelSize = 1.0 / float2(1280.0, 1280.0);
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            shadow += shadowMap.SampleCmpLevelZero(
                sampler_Shadow,
                uv + float2(x, y) * texelSize,
                depth
            );
        }
    }
    return shadow / 9.0;
}

float CalcShadow(float3 worldPos)
{
    // カメラ view-space Z でカスケードを選択
    float viewZ = mul(float4(worldPos, 1.0), CameraView).z;

    if (viewZ < CascadeFar.x)
        return SampleShadowPCF(gShadow0, LightViewProj0, worldPos);
    if (viewZ < CascadeFar.y)
        return SampleShadowPCF(gShadow1, LightViewProj1, worldPos);
    if (viewZ < CascadeFar.z)
        return SampleShadowPCF(gShadow2, LightViewProj2, worldPos);

    return 1.0;
}