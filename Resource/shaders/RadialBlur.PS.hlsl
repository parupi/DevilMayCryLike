#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// 指定した点に向かって伸びる放射状ブラー。
// ヒットした位置を中心に一瞬だけかけて「当たった」衝撃を出す。
cbuffer RadialBlurParam : register(b0)
{
    float32_t2 center;   // ブラーの中心（UV座標 0.0〜1.0）
    float32_t  strength; // 中心へ向かってサンプルを伸ばす量（0.0で無効）
    float32_t  _pad0;
}

static const int kSampleCount = 8;

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // 中心から現在のピクセルへ向かうベクトル。中心から遠いほどブレ幅が大きくなる
    float32_t2 toPixel = input.texcoord - center;

    float32_t4 color = float32_t4(0.0f, 0.0f, 0.0f, 0.0f);
    [unroll]
    for (int i = 0; i < kSampleCount; ++i) {
        // 中心へ近づく方向に少しずつずらしてサンプルする
        float32_t t = (float32_t)i / (float32_t)(kSampleCount - 1);
        float32_t2 uv = saturate(input.texcoord - toPixel * strength * t);
        color += gTexture.Sample(gSampler, uv);
    }

    output.color = color / (float32_t)kSampleCount;

    return output;
}
