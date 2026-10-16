#include "Particle.hlsli"

struct ParticleForGPU
{
    float4x4 WVP;
    float4x4 World;
    float4 color;
    // スプライトシートのコマ切り出し。xy = UVオフセット / zw = UVスケール。
    // コマ分割なしのときは (0,0,1,1) が入るので texcoord はそのまま通る
    float4 uvOffsetScale;
    // x = 粒ごとの乱数 / y = 寿命の進み具合
    float4 misc;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

StructuredBuffer<ParticleForGPU> gParticle : register(t0);

// 光源の一覧（Lighting.hlsli の LightData と同じ並び）。ParticleManager が LightManager から積む
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
    float3 padding; // x=ボリューメトリック / y=パーティクルを照らす倍率 / z=スポットの内側の cos
};
StructuredBuffer<LightData> gLights : register(t3);
cbuffer LightCountCB : register(b2)
{
    uint gLightCount;
};

// 粒の位置が光源から受ける光（仕様書 4.2〜4.4）。見た目を詰めやすい簡易減衰で、
// 半径の半分までは最も明るく、半径で 0 になる（物理的な逆二乗減衰ではない）
float3 CalcParticleLighting(float3 p)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < gLightCount; ++i)
    {
        LightData light = gLights[i];
        if (light.enabled == 0 || light.padding.y <= 0.0f)
            continue;

        float3 toParticle = p - light.position;
        float dist = length(toParticle);
        float attenuation = 0.0f;
        if (light.type == 1)
        {
            attenuation = 1.0f - smoothstep(light.radius * 0.5f, light.radius, dist);
        }
        else if (light.type == 2)
        {
            float3 dir = toParticle / max(dist, 1e-4f);
            float cosToParticle = dot(dir, normalize(light.direction));
            float innerCos = light.padding.z > light.cosAngle ? light.padding.z : 1.0f;
            float angle = smoothstep(light.cosAngle, innerCos, cosToParticle);
            float range = 1.0f - smoothstep(light.distance * 0.5f, light.distance, dist);
            attenuation = angle * range;
        }
        result += light.color.rgb * light.intensity * light.padding.y * attenuation;
    }
    return result;
}

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gParticle[instanceId].WVP);

    float4 uv = gParticle[instanceId].uvOffsetScale;
    output.texcoord = input.texcoord * uv.zw + uv.xy;
    output.meshTexcoord = input.texcoord;

    output.color = gParticle[instanceId].color;
    output.worldPos = mul(input.position, gParticle[instanceId].World).xyz;
    output.misc = gParticle[instanceId].misc;

    // 光は粒の中心で1回だけ計算する（埃は数ピクセルなので画素ごとに計算する必要が無い）
    output.lighting = float3(0.0f, 0.0f, 0.0f);
    if ((flags & kFlagLit) != 0)
    {
        float3 center = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), gParticle[instanceId].World).xyz;
        output.lighting = CalcParticleLighting(center);
    }

    return output;
}
