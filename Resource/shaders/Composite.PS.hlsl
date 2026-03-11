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
        float4 color = forwardCol;
        
        // 前にあるForwardが半透明 → 裏にあるオブジェを描画
        if (forwardCol.a < 1.0f)
        {
            color.rgb = lerp(deferredCol.rgb, forwardCol.rgb, forwardCol.a);
        }
        
        output.color = color;
    }
    else
    {
        // Deferredが前にある → Deferred優先
        float4 color = deferredCol;
        // deferredに描画がない → SkyCubeの描画
        if (deferredDepth >= 1.0f)
        {
            color = forwardCol;
        }
        
        output.color = color;
    }
    
    return output;
}
