#include "Fullscreen.hlsli"

Texture2D<float4> gLighting : register(t0);
SamplerState gSampler : register(s0);

// 露出とトーンマップ（FinalCompositePipeline の b0。RenderEnvironment の値）
cbuffer ToneMapParam : register(b0)
{
    float gExposure;      // 明るさの倍率
    float gToneMapMode;   // 0 = そのまま（1 を超えた所は後で切り捨て）, 1 = 肩だけのカーブ
    float gShoulderStart; // ここまでは素通しし、ここから上をなめらかに 1 へ収める
    float _pad;
};

// 暗部はそのまま通し、明るい所だけを 1 へ漸近させる。境目で傾きが 1 に揃うので折れ目が出ない。
// ACES などは暗部も沈める（トゥがある）ので、ほとんどが暗い洞窟だと暗部が潰れる
float Shoulder(float x, float start)
{
    if (x <= start)
        return x;
    float range = max(1.0f - start, 1e-3f);
    return start + range * (1.0f - exp(-(x - start) / range));
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 color = gLighting.Sample(gSampler, input.texcoord);
    if (gToneMapMode < 0.5f)
    {
        return color;
    }

    float3 rgb = max(color.rgb * gExposure, 0.0f);
    // 色ごとではなく一番明るい成分で縮める。松明のオレンジが白や黄色へ飛ばず色味が残る
    float peak = max(rgb.r, max(rgb.g, rgb.b));
    if (peak > 1e-5f)
    {
        rgb *= Shoulder(peak, saturate(gShoulderStart)) / peak;
    }
    return float4(rgb, color.a);
}