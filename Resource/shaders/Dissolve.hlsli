#ifndef DISSOLVE_COMMON
#define DISSOLVE_COMMON

// --------------------------------------------------
// Dissolve utilities (noise texture based)
// --------------------------------------------------

// threshold >= 0 のとき dissolve を適用する。
// noiseTexture: t1 にバインドされた中心→端方向ノイズテクスチャ (R8_UNORM)
// 戻り値: エッジエミッシブ色（dissolve無効時は float3(0,0,0)）
// discard が必要な場合はこの関数内で行われる。
float3 ApplyDissolve(Texture2D<float> noiseTexture, SamplerState noiseSampler,
                     float2 uv, float threshold, float edgeWidth, float4 edgeColor)
{
    [branch]
    if (threshold < 0.0)
        return float3(0.0, 0.0, 0.0);

    float noiseVal = noiseTexture.Sample(noiseSampler, uv).r;

    if (noiseVal < threshold)
        discard;

    float edgeFactor = 1.0 - saturate((noiseVal - threshold) / max(edgeWidth, 0.001));
    return edgeColor.rgb * edgeFactor * edgeColor.a;
}

#endif // DISSOLVE_COMMON
