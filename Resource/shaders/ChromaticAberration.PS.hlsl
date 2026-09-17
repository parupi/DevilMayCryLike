#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// RGBを中心からの方向にずらして色を分離させる（色収差）。
// ヒットの瞬間や高ランク時に薄くかけると画面の情報量が上がる。
cbuffer ChromaticAberrationParam : register(b0)
{
    float32_t2 center;   // ずらしの中心（UV座標 0.0〜1.0）
    float32_t  strength; // ずらし量（0.0で無効）
    float32_t  _pad0;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // 中心から離れるほど大きくずらす
    float32_t2 offset = (input.texcoord - center) * strength;

    // 赤を外側へ、青を内側へずらす
    float32_t  r    = gTexture.Sample(gSampler, saturate(input.texcoord + offset)).r;
    float32_t4 base = gTexture.Sample(gSampler, input.texcoord);
    float32_t  b    = gTexture.Sample(gSampler, saturate(input.texcoord - offset)).b;

    output.color = float32_t4(r, base.g, b, base.a);

    return output;
}
