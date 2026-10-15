#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 画面全体の色の調整。ColorGradingEffect.h の ColorGradingData と並び順を合わせること
cbuffer ColorGradingParam : register(b0)
{
    float3 gain;      // 色ごとの乗算
    float strength;   // 元の絵と混ぜる割合（0.0で無効）
    float3 lift;      // 色ごとの加算
    float saturation; // 彩度（1=そのまま）
    float contrast;   // コントラスト（1=そのまま）
    float perceptual; // 1 なら見た目の明るさ（ガンマ）の空間で計算する
    float splitBalance; // 暗部とハイライトの境目の明るさ
    float contrastPivot; // コントラストの中心
    float3 shadowTint;  // 暗部に足す色
    float shadowTintStrength;
    float3 highlightTint; // ハイライトに掛ける色
    float highlightTintStrength;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 source = gTexture.Sample(gSampler, input.texcoord);

    if (strength <= 0.0f)
    {
        output.color = source;
        return output;
    }

    // 入力はリニア（ping-pong は SRGB なので読むとリニアに戻る）
    float3 color = source.rgb;
    if (perceptual > 0.5f)
    {
        color = pow(max(color, 0.0f), 1.0f / 2.2f);
    }

    float3 graded = (color - contrastPivot) * contrast + contrastPivot;
    graded = graded * gain + lift;
    float luminance = dot(graded, float3(0.2126f, 0.7152f, 0.0722f));
    graded = lerp(luminance.xxx, graded, saturation);

    // スプリットトーン: 暗い所には色を足し、明るい所には色を掛ける
    float l = saturate(luminance);
    float balance = clamp(splitBalance, 0.05f, 0.95f);
    float shadowWeight = 1.0f - smoothstep(0.0f, balance, l);
    float highlightWeight = smoothstep(balance, 1.0f, l);
    graded += shadowTint * (shadowTintStrength * shadowWeight);
    graded = lerp(graded, graded * highlightTint, saturate(highlightTintStrength * highlightWeight));

    graded = saturate(graded);
    if (perceptual > 0.5f)
    {
        graded = pow(graded, 2.2f);
    }

    output.color = float4(lerp(source.rgb, graded, saturate(strength)), source.a);
    return output;
}
