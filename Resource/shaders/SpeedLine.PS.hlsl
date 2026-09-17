#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 集中線。SpeedLineEffect.h の SpeedLineData と並び順を合わせること
cbuffer SpeedLineParam : register(b0)
{
    float2 center;     // 線が集まる点（UV）
    float strength;    // 線の濃さ（0.0で無効）
    float time;        // 経過時間[s]
    float lineCount;   // 線の本数
    float innerRadius; // この半径より内側には線を出さない
    float aspect;      // 画面の横/縦
    float _pad0;
}

static const float kPi = 3.14159265f;

float Hash(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 source = gTexture.Sample(gSampler, input.texcoord);

    if (strength <= 0.0f)
    {
        output.color = source;
        return output;
    }

    // 縦横比をそろえた空間で測る（線が横長に潰れないように）
    float2 d = float2((input.texcoord.x - center.x) * aspect, input.texcoord.y - center.y);
    float radius = length(d);
    if (radius < 1e-4f)
    {
        output.color = source;
        return output;
    }

    // 角度で線を割り当て、1本ごとに太さ・始まる位置・ちらつきを変える
    float angle = atan2(d.y, d.x);
    float slot = (angle / (2.0f * kPi) + 0.5f) * lineCount;
    float index = floor(slot);
    float inSlot = frac(slot) - 0.5f; // -0.5〜0.5

    float widthRandom = Hash(float2(index, 3.7f));
    float startRandom = Hash(float2(index, 9.1f));
    float flickerRandom = Hash(float2(index, 15.3f));

    // 中心へ向かうほど細くすぼまるくさび形にする
    float halfWidth = lerp(0.12f, 0.42f, widthRandom);
    float lineMask = smoothstep(halfWidth, 0.0f, abs(inSlot));

    // 線ごとに始まる半径を変えて、長さをばらけさせる
    float start = lerp(innerRadius, innerRadius + 0.35f, startRandom);
    float radial = smoothstep(start, start + 0.3f, radius);

    // 細かく明滅させて流れているように見せる
    float flicker = 0.65f + 0.35f * sin(time * 30.0f + flickerRandom * 6.2831f);

    float amount = lineMask * radial * flicker * strength;
    output.color = float4(source.rgb + amount, source.a);
    return output;
}
