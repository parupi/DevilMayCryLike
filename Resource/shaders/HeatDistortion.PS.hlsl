#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 熱の歪み（陽炎）。画面上の帯の範囲だけ、ノイズでUVを揺らす。
// HeatDistortionEffect.h の HeatDistortionData と並び順を合わせること
cbuffer HeatDistortionParam : register(b0)
{
    float2 segmentStart; // 帯の始点（UV）
    float2 segmentEnd;   // 帯の終点（UV）
    float radiusStart;   // 始点の太さ（画面の縦を1とした長さ）
    float radiusEnd;     // 終点の太さ
    float strength;      // UVをずらす量の最大（0.0で無効）
    float time;          // 経過時間[s]
    float aspect;        // 画面の横/縦
    float noiseScale;    // 模様の細かさ
    float scrollSpeed;   // 模様が昇っていく速さ
    float _pad0;
}

float Hash(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

// 格子の角の乱数をなめらかに繋いだノイズ（0〜1）
float ValueNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0f - 2.0f * f);
    float a = Hash(i);
    float b = Hash(i + float2(1.0f, 0.0f));
    float c = Hash(i + float2(0.0f, 1.0f));
    float d = Hash(i + float2(1.0f, 1.0f));
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

// 細かさの違うノイズを重ねて、揺らぎに大小のうねりを出す
float Fbm(float2 p)
{
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int octave = 0; octave < 3; ++octave)
    {
        value += amplitude * ValueNoise(p);
        p *= 2.03f;
        amplitude *= 0.5f;
    }
    return value;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float2 uv = input.texcoord;

    if (strength <= 0.0f)
    {
        output.color = gTexture.Sample(gSampler, uv);
        return output;
    }

    // 縦横比をそろえた空間で帯までの距離を測る（円が楕円にならないように）
    float2 p = float2(uv.x * aspect, uv.y);
    float2 a = float2(segmentStart.x * aspect, segmentStart.y);
    float2 b = float2(segmentEnd.x * aspect, segmentEnd.y);
    float2 ab = b - a;
    float lengthSq = dot(ab, ab);
    float t = (lengthSq > 1e-8f) ? saturate(dot(p - a, ab) / lengthSq) : 0.0f;
    float distanceToSegment = distance(p, a + ab * t);
    float radius = max(lerp(radiusStart, radiusEnd, t), 1e-4f);

    // 帯の中心ほど強く、縁へ向けてなめらかに消す
    float mask = 1.0f - smoothstep(radius * 0.35f, radius, distanceToSegment);
    if (mask <= 0.0f)
    {
        output.color = gTexture.Sample(gSampler, uv);
        return output;
    }

    // 画面の上へ昇っていく揺らぎ（UVのyは下向きなので、時間で足すと模様は上へ流れる）
    float2 q = p * noiseScale + float2(0.0f, time * scrollSpeed);
    float2 offset = float2(Fbm(q) - 0.5f, Fbm(q + float2(5.2f, 1.3f)) - 0.5f) * 2.0f;
    uv = saturate(uv + offset * strength * mask);

    output.color = gTexture.Sample(gSampler, uv);
    return output;
}
