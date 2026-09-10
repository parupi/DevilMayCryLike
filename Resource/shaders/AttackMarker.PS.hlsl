// 敵の攻撃予兆マーカー。地面に寝かせた四角形の上へ、図形を手続き的に描く。
//
// 読み取ってほしい情報は2つ。
//   1. どこに当たるか … 図形の外枠（範囲そのもの）
//   2. いつ当たるか   … 内側の塗りが外枠へ広がりきった瞬間に判定が出る
// テクスチャは使わないので、形も枠の太さもワールド単位[m]で一定に見える。

struct PSInput
{
    float4 position : SV_POSITION;
    float2 local    : TEXCOORD0;
    float4 param    : TEXCOORD1;
    float4 extent   : TEXCOORD2;
    float4 color    : COLOR;
};

// param.x に入る形状。C++ 側の TelegraphShape と並びを合わせること
static const float kShapeCircle = 0.0f; // 円（中心から外へ広がる）
static const float kShapeFan    = 1.0f; // 扇（敵の正面へ開く）
static const float kShapeRect   = 2.0f; // 矩形（敵の正面へ伸びる帯）

static const float kBaseFill    = 0.16f; // 範囲全体に薄く敷く赤。まだ来ていない場所も見える
static const float kFillGain    = 0.42f; // 塗りつぶされた側の濃さ
static const float kLeadGain    = 1.40f; // 塗りの先端に走る光の帯の強さ
static const float kLeadWidth   = 0.45f; // その帯の太さ[m]
static const float kOutlineGain = 1.30f; // 外枠の強さ

// 扇形（原点が要・+y 方向へ半角 a で開く・半径 r）の符号付き距離[m]。
// 「半径で切ってから角度でも切る」式だと要の付近で距離の意味が壊れて穴が空くので、
// 勾配の大きさが 1 に保たれるこの式を使う
float SdPie(float2 p, float a, float r)
{
    p.x = abs(p.x);
    const float2 c = float2(sin(a), cos(a));
    const float l = length(p) - r;
    const float m = length(p - c * clamp(dot(p, c), 0.0f, r));
    return max(l, m * sign(c.y * p.x - c.x * p.y));
}

float4 main(PSInput input) : SV_TARGET
{
    const float shape     = input.param.x;
    const float progress  = saturate(input.param.y);
    const float halfAngle = input.param.z;
    const float alpha     = input.param.w;

    const float2 extent = input.extent.xy;
    const float  flash  = input.extent.z;
    const float  edgeW  = input.extent.w;

    const float2 p = input.local;

    float sd;      // 図形の符号付き距離[m]（負が内側）
    float fill;    // 塗りの進み具合を測る座標[m]
    float fillMax; // fill の最大値[m]

    if (shape >= kShapeRect - 0.5f) {
        // 矩形。手前(-y)から奥(+y)へ塗られる＝敵から遠ざかる向きに走る
        const float2 q = abs(p) - extent;
        sd = min(max(q.x, q.y), 0.0f) + length(max(q, 0.0f));
        fill = p.y + extent.y;
        fillMax = extent.y * 2.0f;
    } else if (shape >= kShapeFan - 0.5f) {
        // 扇。要（敵の足元）から外へ塗られる
        sd = SdPie(p, halfAngle, extent.x);
        fill = length(p);
        fillMax = extent.x;
    } else {
        // 円。中心から外へ塗られる
        fill = length(p);
        sd = fill - extent.x;
        fillMax = extent.x;
    }

    // aa は画面1ピクセルが覆うワールド距離[m]。縁のギザギザを消すのに使う。
    // 地面を浅い角度で見ると 1ピクセルが数十cmを覆うので、枠の側は太らせすぎないよう抑える
    const float aa = max(fwidth(sd), 1.0e-4f);
    const float edgeAA = min(aa, edgeW);

    // 図形の外は描かない
    const float inside = 1.0f - smoothstep(-aa, aa, sd);
    if (inside <= 0.002f) discard;

    // ── 1) 攻撃範囲そのもの。薄く敷いて「ここが危ない」を先に見せる
    float intensity = kBaseFill;

    // ── 2) 発生タイミング。塗りが外枠へ届いた瞬間に判定が出る
    const float front  = progress * fillMax;
    const float fillAA = max(fwidth(fill), 1.0e-4f);
    const float filled = 1.0f - smoothstep(front - fillAA, front + fillAA, fill);
    intensity += filled * kFillGain;

    // ── 3) 塗りの先端に走る光の帯。今どこまで進んだかを一目で読めるようにする
    const float leadT = (fill - front) / kLeadWidth;
    intensity += exp(-leadT * leadT) * kLeadGain;

    // ── 4) 外枠。発生が近づくほど明るくして緊張感を出す
    const float outline = 1.0f - smoothstep(edgeW - edgeAA, edgeW + edgeAA, abs(sd));
    intensity += outline * kOutlineGain * (0.55f + 0.95f * progress);

    intensity *= inside;

    // ── 5) 判定が出た瞬間の閃光。白く飛ばしてから消える
    float3 color = input.color.rgb;
    color = lerp(color, float3(1.0f, 0.92f, 0.82f), saturate(flash) * 0.8f);
    intensity += flash * inside * (0.7f + outline * 1.5f);

    // 乗算済みアルファ（SrcBlend=ONE / DestBlend=INV_SRC_ALPHA）。
    // 加算だけだと青い床の上で赤がピンクに浮くので、地面の色を押しのける形にする。
    // rgb は 1 を超えてよく、明るい枠はブルームが拾う
    const float coverage = saturate(intensity) * alpha;
    return float4(color * (intensity * alpha), coverage);
}
