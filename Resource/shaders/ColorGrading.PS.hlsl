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
    float3 _pad0;
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

    float3 graded = (source.rgb - 0.5f) * contrast + 0.5f;
    graded = graded * gain + lift;
    float luminance = dot(graded, float3(0.2126f, 0.7152f, 0.0722f));
    graded = lerp(luminance.xxx, graded, saturation);

    output.color = float4(lerp(source.rgb, saturate(graded), saturate(strength)), source.a);
    return output;
}
