#include "Fullscreen.hlsli"

Texture2D gDeferredColor : register(t0); // Deferred Lighting結果
Texture2D gDeferredDepth : register(t1); // Deferred Depth
Texture2D gForwardColor : register(t2); // Forward カラーバッファ
Texture2D gForwardDepth : register(t3); // Forward Depth

SamplerState samLinear : register(s0);

// クリップ空間深度→NDCの値はそのままでOK (0～1)
float LinearizeDepth(float depth)
{
    return depth; // 必要なら再線形化式に更新
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float4 deferredCol = gDeferredColor.Sample(samLinear, input.texcoord);
    float deferredDepth = LinearizeDepth(gDeferredDepth.Sample(samLinear, input.texcoord).r);

    float4 forwardCol = gForwardColor.Sample(samLinear, input.texcoord);
    float forwardDepth = LinearizeDepth(gForwardDepth.Sample(samLinear, input.texcoord).r);

    // -------------------------
    // Depth比較で前後判定
    // -------------------------
    bool forwardIsInFront = (forwardDepth < deferredDepth);

    // -------------------------
    // アルファブレンド調整
    // α=1でも奥にある物は隠れない
    // -------------------------
    if (forwardIsInFront)
    {
        // Forward が前にある → Forward優先
        output.color = forwardCol;
    }
    else
    {
        // Deferred が前にある → Deferred優先
        // Forward のアルファだけ反映 (半透明物体対応)
        float3 blended = lerp(deferredCol.rgb, forwardCol.rgb, forwardCol.a);
        output.color = float4(blended, 1.0f);
        
        output.color = deferredCol;
    }
    
    return output;
}
