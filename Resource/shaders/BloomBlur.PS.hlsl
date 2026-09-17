#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// ブルームの2・3パス目。分離ガウスブラーなので横・縦の2回に分けて呼ぶ。
cbuffer BloomBlurParam : register(b0)
{
    float32_t2 direction; // 1タップあたりのUVオフセット（横なら(x,0)、縦なら(0,y)）
    float32_t  _pad0;
    float32_t  _pad1;
}

// 中心 + 片側4タップの9タップガウス
static const int kTapCount = 5;
static const float32_t kWeights[kTapCount] = {
    0.227027f, 0.194594f, 0.121621f, 0.054054f, 0.016216f
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float32_t3 color = gTexture.Sample(gSampler, input.texcoord).rgb * kWeights[0];

    [unroll]
    for (int i = 1; i < kTapCount; ++i) {
        float32_t2 offset = direction * (float32_t)i;
        color += gTexture.Sample(gSampler, saturate(input.texcoord + offset)).rgb * kWeights[i];
        color += gTexture.Sample(gSampler, saturate(input.texcoord - offset)).rgb * kWeights[i];
    }

    output.color = float32_t4(color, 1.0f);

    return output;
}
