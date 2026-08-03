#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// ブルームの最終パス。ぼかした光を元の絵に「加算ブレンドで」重ねる。
// 元の絵は先にコピー済みで、このシェーダはブルーム成分だけを出力する。
cbuffer BloomCompositeParam : register(b0)
{
    float32_t intensity; // 光の強さ
    float32_t _pad0;
    float32_t _pad1;
    float32_t _pad2;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float32_t3 bloom = gTexture.Sample(gSampler, input.texcoord).rgb;

    // アルファは加算しない（ブレンド設定で DestAlpha を維持している）
    output.color = float32_t4(bloom * intensity, 0.0f);

    return output;
}
