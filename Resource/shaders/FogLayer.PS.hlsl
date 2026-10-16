#include "FogLayer.hlsli"

Texture2D<float4> gSceneWorldPos : register(t0);

// ---- 手続きノイズ（テクスチャのタイル跡が出ないように） ----
float Hash(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

float ValueNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0f - 2.0f * f);
    float a = Hash(i);
    float b = Hash(i + float2(1.0f, 0.0f));
    float c = Hash(i + float2(0.0f, 1.0f));
    float d = Hash(i + float2(1.0f, 1.0f));
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

// 4オクターブ。細かい成分ほど少し回しながら重ねて、格子の向きを目立たなくする
float Fbm(float2 p)
{
    const float2x2 rot = float2x2(0.8f, -0.6f, 0.6f, 0.8f);
    float value = 0.0f;
    float amplitude = 0.5f;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        value += amplitude * ValueNoise(p);
        p = mul(rot, p) * 2.03f;
        amplitude *= 0.5f;
    }
    return value;
}

float4 main(FogVSOutput input) : SV_TARGET
{
    // 濃淡: 流れる大きな模様と、逆向きに少し速く流れる細かい模様を混ぜて、うねって見えるようにする
    float2 p = input.worldPos.xz * gNoiseScale + gLayerSeed;
    float coarse = Fbm(p + gNoiseOffset * gNoiseScale);
    float fine = Fbm(p * 2.3f - gNoiseOffset * gNoiseScale * 1.7f + 11.7f);
    float n = coarse * 0.7f + fine * 0.3f;
    float alpha = gOpacity * smoothstep(0.3f, 0.75f, n);

    // 板の端を薄くする。壁の上を越えた先で端が見えるので、直線的に薄くすると
    // 薄くし始める所に折れ目の線が出る。なめらかな曲線で、角も丸めて薄くする
    float2 edge2 = 1.0f - abs(input.local);
    float edgeFade = max(gEdgeFade, 1e-3f);
    alpha *= smoothstep(0.0f, edgeFade, edge2.x) * smoothstep(0.0f, edgeFade, edge2.y);

    // 真横から見ると板だと分かるので、視線が水平に近いほど薄くする
    float3 toPixel = input.worldPos - gCameraPos;
    float distanceToPixel = length(toPixel);
    float facing = abs(toPixel.y) / max(distanceToPixel, 1e-3f);
    alpha *= lerp(1.0f, saturate(facing * 4.0f), gGrazingFade);

    // 壁や岩と交わる所を薄くする（ソフトパーティクルと同じ）。何も無い画素（w=0）はそのまま
    float4 scene = gSceneWorldPos.Load(int3(input.position.xy, 0));
    if (scene.w > 0.5f)
    {
        float sceneDistance = distance(gCameraPos, scene.xyz);
        alpha *= saturate((sceneDistance - distanceToPixel) / max(gSoftDistance, 1e-3f));
    }

    if (alpha <= 0.001f)
        discard;

    // 乗算済みアルファ
    return float4(gColor * alpha, alpha);
}
