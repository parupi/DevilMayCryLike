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

// 画面全体の空気（RenderEnvironment.h の EnvironmentCB と並びを合わせること）
cbuffer EnvironmentCB : register(b5)
{
    float3 BackgroundColor;  // 何も描かれていない画素の色（リニア）
    float _envPad0;
    float3 AmbientColor;     // 環境光
    float AmbientAlbedoMix;  // 環境光にアルベドを掛ける割合（0=そのまま足す）
    float3 DirectionalScale; // 平行光に掛ける色
    float _envPad2;
    float DarkStartHeight;   // 高さによる暗化（環境オブジェクトだけ）
    float DarkEndHeight;
    float DarkStrength;      // 0 で無効
    float _envPad3;
    float3 HeightFogColor;   // 高さフォグの色（リニア）
    float HeightFogDensity;  // 基準の高さでの濃さ（1mあたり）。0 で無効
    float HeightFogBaseHeight;
    float HeightFogFalloff;  // 1m 上がるごとの濃さの伸び（指数）
    float HeightFogMaxOpacity;
    float _envPad4;
    float3 DistanceFogColor;  // 距離フォグの色（リニア）
    float DistanceFogDensity; // 0 で無効
    float DistanceFogStart;   // これより近い所には掛けない[m]
    float DistanceFogMaxOpacity;
    float2 _envPad5;
    float3 CharacterAmbient; // キャラだけに足す環境光（アルベドに掛ける）
    float RimPower;
    float3 RimColor;         // キャラの輪郭光（0 で無効）
    float _envPad6;
}

// 距離フォグの量（0〜1）。仕様書 3.1 の 1 - exp(-density * distance) に、掛け始める距離と上限を足したもの
float CalcDistanceFog(float3 cameraPos, float3 P)
{
    if (DistanceFogDensity <= 0.0f)
        return 0.0f;
    float d = max(length(P - cameraPos) - DistanceFogStart, 0.0f);
    return min(1.0f - exp(-DistanceFogDensity * d), DistanceFogMaxOpacity);
}

// 高さフォグ。濃さ ρ(y) = d·exp(k·(y - y0)) の霧を、カメラから P までの線分で積分した量を返す（0〜1）。
// ∫ρ ds = d·L·exp(k·(c.y - y0))·(exp(k·Δy) - 1)/(k·Δy)   （Δy = P.y - c.y, L = |P - c|）
float CalcHeightFog(float3 cameraPos, float3 P)
{
    if (HeightFogDensity <= 0.0f)
        return 0.0f;
    float L = length(P - cameraPos);
    float dy = P.y - cameraPos.y;
    float kdy = HeightFogFalloff * dy;
    // Δy がほぼ0（水平に見ている）なら (e^x - 1)/x → 1
    float grow = abs(kdy) > 1e-4f ? (exp(clamp(kdy, -30.0f, 30.0f)) - 1.0f) / kdy : 1.0f;
    float base = exp(clamp(HeightFogFalloff * (cameraPos.y - HeightFogBaseHeight), -30.0f, 30.0f));
    float amount = HeightFogDensity * L * base * grow;
    return min(1.0f - exp(-amount), HeightFogMaxOpacity);
}

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