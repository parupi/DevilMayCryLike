#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// ブルームの1パス目。しきい値を超えた明るい部分だけを抜き出す。
cbuffer BloomBrightParam : register(b0)
{
    float32_t threshold; // この輝度を超えた部分が光る
    float32_t softKnee;  // しきい値付近をなめらかに繋ぐ幅（0だと境界がくっきり出る）
    float32_t _pad0;
    float32_t _pad1;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float32_t3 color = gTexture.Sample(gSampler, input.texcoord).rgb;

    // 人の目の感度に合わせた輝度
    float32_t luminance = dot(color, float32_t3(0.2126f, 0.7152f, 0.0722f));

    // しきい値からどれだけ超えているかを 0〜1 で求める。
    // softKnee を広げるとしきい値付近がじわっと光り始める
    float32_t knee = max(softKnee, 1e-4f);
    float32_t contribution = saturate((luminance - threshold) / knee);

    output.color = float32_t4(color * contribution, 1.0f);

    return output;
}
