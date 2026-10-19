#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// FXAA の設定（FxaaEffect.h の FxaaData と並びを合わせること）
cbuffer FxaaParam : register(b0)
{
    float2 gRcpFrame;         // 1 / 画面の大きさ（ping-pong の実サイズ）
    float gSubpix;            // 1ピクセルより細いもの（髪・剣先）をどれだけ均すか。0 で無効、1 で最大
    float gEdgeThreshold;     // 周りとの明るさの差がこの割合を超えたらエッジとみなす（小さいほど多く拾う）
    float gEdgeThresholdMin;  // 暗い所で拾わないための下限（暗部のノイズまでぼかさない）
    float gShowEdges;         // 1 ならエッジと判定した所を赤で出す（調整用）
    float2 _pad;
};

// 探索の歩幅。FXAA 3.11 Quality のプリセット 12 相当（近くは細かく、遠くは大股）
static const int kSearchSteps = 12;
static const float kSearchStep[kSearchSteps] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 8.0f };

// サンプラーは WRAP なので、画面の端で反対側の色を拾わないよう内側に収める
float3 SampleColor(float2 uv)
{
    uv = clamp(uv, gRcpFrame * 0.5f, 1.0f - gRcpFrame * 0.5f);
    return gTexture.SampleLevel(gSampler, uv, 0).rgb;
}

// バッファの中身はリニアの色なので、sqrt で見た目の明るさに近づけてから比べる。
// リニアのままだと暗い所の段差が小さく出て、洞窟の暗部のエッジを拾えない
float Luma(float3 rgb)
{
    return sqrt(dot(saturate(rgb), float3(0.299f, 0.587f, 0.114f)));
}

float LumaAt(float2 uv)
{
    return Luma(SampleColor(uv));
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    const float2 uv = input.texcoord;
    const float2 r = gRcpFrame;

    float4 center = gTexture.SampleLevel(gSampler, uv, 0);
    float lumaM = Luma(center.rgb);
    // UV は下向きが +y。N=上 / S=下
    float lumaN = LumaAt(uv + float2(0.0f, -r.y));
    float lumaS = LumaAt(uv + float2(0.0f, r.y));
    float lumaW = LumaAt(uv + float2(-r.x, 0.0f));
    float lumaE = LumaAt(uv + float2(r.x, 0.0f));

    // --- 1. エッジかどうか。周り4つとの明るさの幅が小さければそのまま返す ---
    float rangeMax = max(lumaM, max(max(lumaN, lumaS), max(lumaW, lumaE)));
    float rangeMin = min(lumaM, min(min(lumaN, lumaS), min(lumaW, lumaE)));
    float range = rangeMax - rangeMin;
    if (range < max(gEdgeThresholdMin, rangeMax * gEdgeThreshold))
    {
        return center;
    }

    float lumaNW = LumaAt(uv + float2(-r.x, -r.y));
    float lumaNE = LumaAt(uv + float2(r.x, -r.y));
    float lumaSW = LumaAt(uv + float2(-r.x, r.y));
    float lumaSE = LumaAt(uv + float2(r.x, r.y));

    // --- 2. 1ピクセルより細いものの均し量（周り8つの平均との差が大きいほど強く） ---
    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float lumaL = (2.0f * (lumaNS + lumaWE) + (lumaNW + lumaNE + lumaSW + lumaSE)) / 12.0f;
    float subpixC = saturate(abs(lumaL - lumaM) / range);
    subpixC = (-2.0f * subpixC + 3.0f) * subpixC * subpixC;
    float subpixF = subpixC * subpixC * gSubpix;

    // --- 3. エッジの向き。横の線（上下で明るさが変わる）か縦の線か ---
    float edgeHorz = abs(-2.0f * lumaW + lumaNW + lumaSW)
                   + abs(-2.0f * lumaM + lumaNS) * 2.0f
                   + abs(-2.0f * lumaE + lumaNE + lumaSE);
    float edgeVert = abs(-2.0f * lumaN + lumaNW + lumaNE)
                   + abs(-2.0f * lumaM + lumaWE) * 2.0f
                   + abs(-2.0f * lumaS + lumaSW + lumaSE);
    bool horzSpan = edgeHorz >= edgeVert;

    if (gShowEdges > 0.5f)
    {
        // 横の線は赤、縦の線は黄色
        return horzSpan ? float4(1.0f, 0.0f, 0.0f, center.a) : float4(1.0f, 1.0f, 0.0f, center.a);
    }

    // エッジをまたぐ向き（横の線なら上下）。差が大きい側へ半ピクセルずれた所がエッジの上
    float lengthSign = horzSpan ? r.y : r.x;
    float luma0 = horzSpan ? lumaN : lumaW;
    float luma1 = horzSpan ? lumaS : lumaE;
    float grad0 = abs(luma0 - lumaM);
    float grad1 = abs(luma1 - lumaM);
    bool pair0 = grad0 >= grad1;
    float gradientScaled = max(grad0, grad1) * 0.25f;
    float lumaLocalAvg;
    if (pair0)
    {
        lengthSign = -lengthSign;
        lumaLocalAvg = 0.5f * (luma0 + lumaM);
    }
    else
    {
        lumaLocalAvg = 0.5f * (luma1 + lumaM);
    }

    float2 posB = uv;
    if (horzSpan)
        posB.y += lengthSign * 0.5f;
    else
        posB.x += lengthSign * 0.5f;

    // --- 4. エッジに沿って両方向へ、明るさが変わる所（線の端）まで歩く ---
    float2 offNP = horzSpan ? float2(r.x, 0.0f) : float2(0.0f, r.y);
    float2 posN = posB - offNP;
    float2 posP = posB + offNP;
    float lumaEndN = LumaAt(posN) - lumaLocalAvg;
    float lumaEndP = LumaAt(posP) - lumaLocalAvg;
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;

    [loop]
    for (int i = 1; i < kSearchSteps && !(doneN && doneP); ++i)
    {
        if (!doneN)
        {
            posN -= offNP * kSearchStep[i];
            lumaEndN = LumaAt(posN) - lumaLocalAvg;
            doneN = abs(lumaEndN) >= gradientScaled;
        }
        if (!doneP)
        {
            posP += offNP * kSearchStep[i];
            lumaEndP = LumaAt(posP) - lumaLocalAvg;
            doneP = abs(lumaEndP) >= gradientScaled;
        }
    }

    // --- 5. 近い方の端からの距離で、どれだけエッジの向こう側の色を混ぜるかを決める ---
    float dstN = horzSpan ? (uv.x - posN.x) : (uv.y - posN.y);
    float dstP = horzSpan ? (posP.x - uv.x) : (posP.y - uv.y);
    bool lumaMLTZero = (lumaM - lumaLocalAvg) < 0.0f;
    bool goodSpanN = (lumaEndN < 0.0f) != lumaMLTZero;
    bool goodSpanP = (lumaEndP < 0.0f) != lumaMLTZero;
    bool directionN = dstN < dstP;
    float dst = min(dstN, dstP);
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    float spanLength = max(dstN + dstP, 1e-6f);
    float pixelOffset = goodSpan ? (0.5f - dst / spanLength) : 0.0f;
    float finalOffset = max(pixelOffset, subpixF);

    float2 finalUv = uv;
    if (horzSpan)
        finalUv.y += finalOffset * lengthSign;
    else
        finalUv.x += finalOffset * lengthSign;

    return float4(SampleColor(finalUv), center.a);
}
