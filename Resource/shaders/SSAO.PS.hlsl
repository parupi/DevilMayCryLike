#include "Fullscreen.hlsli"

// SSAO（SSAORenderPass）。SSAORenderPass.h の SSAOParam と並びを合わせること
cbuffer SSAOParam : register(b0)
{
    float4x4 gViewProj;
    float3 gCameraPos;
    float gRadius;    // サンプルを散らす半径[m]
    float gBias;      // 自分の面を遮蔽と数えないための余裕[m]
    float gIntensity; // 遮蔽の強さ
    float gPower;     // 仕上げのカーブ（1 で素直）
    float _pad0;
};

Texture2D<float4> gNormal : register(t0);   // GBuffer の法線（0〜1 に詰めたもの）
Texture2D<float4> gWorldPos : register(t1); // GBuffer のワールド座標（w=0 は何も無い）

// 法線方向を z とした半球の中の点。中心に寄せてあるので、近い所の遮蔽ほど多く拾う
static const int kSampleCount = 12;
static const float3 kKernel[kSampleCount] =
{
    float3(-0.1369f, 0.0115f, 0.0738f),
    float3(-0.1700f, 0.0029f, 0.0350f),
    float3(-0.0300f, -0.1942f, 0.0513f),
    float3(-0.0514f, 0.2226f, 0.0869f),
    float3(0.1145f, -0.1052f, 0.2537f),
    float3(0.2294f, -0.0921f, 0.2651f),
    float3(0.1673f, -0.0915f, 0.3957f),
    float3(0.3906f, 0.2640f, 0.2372f),
    float3(0.1030f, 0.0349f, 0.6186f),
    float3(-0.7155f, -0.0227f, 0.1885f),
    float3(0.3253f, 0.5116f, 0.6159f),
    float3(0.3189f, 0.2700f, 0.9085f)
};

// 画素ごとのばらつき（Interleaved Gradient Noise）。サンプルの向きを回して縞を消し、ぼかしで均す
float InterleavedGradientNoise(float2 pixel)
{
    return frac(52.9829189f * frac(dot(pixel, float2(0.06711056f, 0.00583715f))));
}

float main(VertexShaderOutput input) : SV_TARGET
{
    int2 pixel = int2(input.position.xy);
    float4 worldPos = gWorldPos.Load(int3(pixel, 0));
    if (worldPos.w < 0.5f)
    {
        return 1.0f;
    }
    float3 P = worldPos.xyz;
    float3 N = normalize(gNormal.Load(int3(pixel, 0)).xyz * 2.0f - 1.0f);

    // 法線を z とする向きを組み、画素ごとに回す
    float angle = InterleavedGradientNoise(input.position.xy) * 6.2831853f;
    float3 up = abs(N.y) < 0.99f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);
    float c = cos(angle);
    float s = sin(angle);
    float3 T2 = T * c + B * s;
    float3 B2 = cross(N, T2);

    uint width;
    uint height;
    gWorldPos.GetDimensions(width, height);

    float occlusion = 0.0f;
    [unroll]
    for (int i = 0; i < kSampleCount; ++i)
    {
        float3 k = kKernel[i];
        float3 S = P + (T2 * k.x + B2 * k.y + N * k.z) * gRadius;

        float4 clip = mul(float4(S, 1.0f), gViewProj);
        if (clip.w <= 1e-4f)
            continue;
        float2 ndc = clip.xy / clip.w;
        float2 uv = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);
        if (any(uv < 0.0f) || any(uv > 1.0f))
            continue;

        float4 scene = gWorldPos.Load(int3(uv * float2(width, height), 0));
        if (scene.w < 0.5f)
            continue;

        // サンプル点より手前に何かが描かれていれば遮られている。
        // ただし離れすぎた物（手前を横切る柱など）は数えない
        float sampleDistance = distance(gCameraPos, S);
        float sceneDistance = distance(gCameraPos, scene.xyz);
        float rangeCheck = saturate(gRadius / max(distance(scene.xyz, P), 1e-3f));
        occlusion += (sceneDistance < sampleDistance - gBias ? 1.0f : 0.0f) * rangeCheck;
    }

    float ao = 1.0f - (occlusion / kSampleCount) * gIntensity;
    return pow(saturate(ao), gPower);
}
