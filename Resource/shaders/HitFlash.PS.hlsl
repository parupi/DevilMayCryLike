#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// 画面全体を一瞬光らせる。加算なので明るい部分から白く飛んで打撃の衝撃が出る。
cbuffer HitFlashParam : register(b0)
{
    float32_t4 flashColor; // rgb = 光の色, a = 強さ（0.0で無効）
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);

    output.color.rgb = saturate(texColor.rgb + flashColor.rgb * flashColor.a);
    output.color.a = texColor.a;

    return output;
}
