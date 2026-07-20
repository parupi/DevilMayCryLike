#include "GBuffer.hlsli"
#include "Dissolve.hlsli"

Texture2D baseColorMap : register(t0);
Texture2D<float> gDissolveNoise : register(t1);
SamplerState samLinear : register(s0);

cbuffer MaterialParam : register(b0)
{
    float roughness;
    float metal;
    float dissolveThreshold; // -1.0 = disabled, 0.0-1.0 = dissolve amount
    float dissolveEdgeWidth; // edge glow width in noise space

    float4x4 uvTransform;

    float4 dissolveEdgeColor; // rgb = emissive color, a = intensity multiplier
};

// レンダラー単位のDissolve上書き + エミッシブティント（ルート定数）。
// マテリアル(=モデル)は複数オブジェクトで共有されるため、
// 個別オブジェクトを溶かしたり発光させたりする場合はこちらをドロー毎に設定する。
cbuffer DissolveOverride : register(b2)
{
    float4 gDissolveOverrideParam;     // x: threshold (-0.5未満で無効=マテリアル値を使用), y: edgeWidth
    float4 gDissolveOverrideEdgeColor; // rgb = emissive color, a = intensity multiplier
    float4 gEmissiveTint;              // rgb = 加算する発光色, a = 強度(0で無効)
};

GBufferOutput main(VSOutput input)
{
    GBufferOutput output;

    float2 uv = input.uv_and_pad.xy;
    float2 transformedUV = mul(float4(uv, 0.0f, 1.0f), uvTransform).xy;

    // ------- Dissolve -------
    float threshold = dissolveThreshold;
    float edgeWidth = dissolveEdgeWidth;
    float4 edgeColor = dissolveEdgeColor;
    [branch]
    if (gDissolveOverrideParam.x > -0.5)
    {
        threshold = gDissolveOverrideParam.x;
        edgeWidth = gDissolveOverrideParam.y;
        edgeColor = gDissolveOverrideEdgeColor;
    }
    float3 edgeEmissive = ApplyDissolve(gDissolveNoise, samLinear, uv, threshold, edgeWidth, edgeColor);

    // ------- Albedo -------
    float4 baseColor = baseColorMap.Sample(samLinear, transformedUV);
    // エミッシブティント（スーパーアーマーの紫発光など、レンダラー単位の一時発光）
    float3 tintEmissive = gEmissiveTint.rgb * gEmissiveTint.a;
    output.baseColor_Roughness = float4(baseColor.rgb + edgeEmissive + tintEmissive, roughness);

    // ------- Normal -------
    float3 normalWS = normalize(input.normalWS.xyz);
    float3 packedNormal = normalWS * 0.5f + 0.5f;
    output.normal_Metal = float4(packedNormal, metal);

    // ------- World Pos -------
    output.worldPos_Padding = input.worldPosWS;

    return output;
}
