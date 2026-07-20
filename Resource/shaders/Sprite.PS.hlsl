#include "Sprite.hlsli"
#include "Dissolve.hlsli"

cbuffer SpriteMaterial : register(b0)
{
    float4 color;
    float4x4 uvTransform;
    float dissolveThreshold; // -1.0 = disabled, 0.0-1.0 = dissolve amount
    float dissolveEdgeWidth; // edge glow width in noise space
    float radialFill;        // -1.0 = disabled, 0.0-1.0 = 表示割合（上から時計回りに欠ける円形ゲージ）
    float padding;
    float4 dissolveEdgeColor; // rgb = emissive color, a = intensity multiplier
};

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gDissolveNoise : register(t1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (textureColor.a == 0.0f)
        discard;

    // ------- Radial fill（上から時計回りに欠ける円形ゲージ。ロックオンレティクルのHP表示など） -------
    [branch]
    if (radialFill >= 0.0)
    {
        const float PI2 = 6.28318530718;
        float2 dir = input.texcoord - float2(0.5, 0.5);
        // 上(12時)を0として時計回りに増える角度 [0, 2π)
        float angle = atan2(dir.x, -dir.y);
        if (angle < 0.0)
            angle += PI2;
        // 上から時計回りに (1 - radialFill) 分の弧を消す
        if (angle < (1.0 - radialFill) * PI2)
            discard;
    }

    // ------- Dissolve -------
    float3 edgeEmissive = ApplyDissolve(gDissolveNoise, gSampler, input.texcoord, dissolveThreshold, dissolveEdgeWidth, dissolveEdgeColor);

    PixelShaderOutput output;
    output.color = color * textureColor;
    output.color.rgb += edgeEmissive;

    if (output.color.a == 0.0f)
        discard;

    return output;
}
