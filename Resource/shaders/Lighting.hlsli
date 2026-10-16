// Lighting.hlsli
struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Sampler
SamplerState sampler_Linear : register(s0);

// GBuffer textures
Texture2D<float4> gAlbedo : register(t0);
Texture2D<float4> gNormal : register(t1);
Texture2D<float4> gWorldPos : register(t2);
Texture2D<float4> gMaterial : register(t3);

// Directional light CB (例: register b1)
struct LightData
{
    uint type; // 0:Directional 1:Point 2:Spot
    uint enabled;
    float intensity;
    float decay;

    float4 color;

    float3 position;
    float radius;

    float3 direction;
    float cosAngle;

    float distance;
    float3 padding; // x = ボリューメトリックライトの倍率（BaseLight::SetVolumetricScale）
};

StructuredBuffer<LightData> Lights : register(t4);

// Camera CB -> **register(b2)**
cbuffer CameraCB : register(b2)
{
    float4 CameraPosWS; // xyz = camera world position, w = padding
}

// Light count
cbuffer LightCountCB : register(b3)
{
    uint LightCount;
};

// 画面全体の空気（RenderEnvironment.h の EnvironmentCB と並びを合わせること）
cbuffer EnvironmentCB : register(b5)
{
    float3 BackgroundColor;  // 何も描かれていない画素の色（リニア）
    float _envPad0;
    float3 AmbientColor;     // 環境光
    float AmbientAlbedoMix;  // 環境光にアルベドを掛ける割合（0=そのまま足す）
    float3 DirectionalScale; // 平行光に掛ける色
    float _envPad2;
    float DarkStartHeight;   // 高さによる暗化（環境オブジェクトだけ）
    float DarkEndHeight;
    float DarkStrength;      // 0 で無効
    float _envPad3;
    float3 HeightFogColor;   // 高さフォグの色（リニア）
    float HeightFogDensity;  // 基準の高さでの濃さ（1mあたり）。0 で無効
    float HeightFogBaseHeight;
    float HeightFogFalloff;  // 1m 上がるごとの濃さの伸び（指数）
    float HeightFogMaxOpacity;
    float _envPad4;
    float3 DistanceFogColor;  // 距離フォグの色（リニア）
    float DistanceFogDensity; // 0 で無効
    float DistanceFogStart;   // これより近い所には掛けない[m]
    float DistanceFogMaxOpacity;
    float2 _envPad5;
    float3 CharacterAmbient; // キャラだけに足す環境光（アルベドに掛ける）
    float RimPower;
    float3 RimColor;         // キャラの輪郭光（0 で無効）
    float _envPad6;
    float SsaoAmbientStrength; // AO を環境光に掛ける割合（0 で無効）
    float SsaoDirectStrength;  // AO を直接光に掛ける割合（0 で無効）
    float VolumetricStrength;  // 空気中の光の散乱の強さ（0 で無効）
    float VolumetricMaxDistance; // 背景の画素で視線を積分する距離[m]
    float4x4 InvViewProj;      // 背景の画素の視線方向を出すのに使う
    float SpotVolumetricStrength; // スポットライトの光の筋の強さ（0 で無効）
    float SpotVolumetricSteps;    // 1本の視線あたりのサンプル数
    float SpotFogNoiseScale;      // 霧の濃淡の細かさ（1mあたり）
    float SpotFogNoiseInfluence;  // 濃淡の強さ（0=一様）
    float3 SpotFogNoiseSpeed;     // 霧の濃淡が流れる速さ[m/秒]
    float SpotPhaseG;             // 前方散乱の強さ（Henyey-Greenstein の g）
    float EnvTime;                // 霧の濃淡を流す時計[秒]
    float SpotApexFade;           // 光源からこの距離[m]までは筋を薄くする
    float DebugView;              // 0=通常 / 1=光の筋だけ / 2=埃の受けた光だけ / 3=埃の透明度（2・3 は背景を黒に）
    float _envPad7;
}

// ---- スポットライトの光の筋（視線と円錐の重なる区間だけをレイマーチする） ----

float VolHash(float3 p)
{
    p = frac(p * 0.3183099f + 0.1f);
    p *= 17.0f;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 3D の値ノイズ（0〜1）。霧の濃淡に使う
float VolNoise(float3 x)
{
    float3 i = floor(x);
    float3 f = frac(x);
    f = f * f * (3.0f - 2.0f * f);
    return lerp(lerp(lerp(VolHash(i + float3(0, 0, 0)), VolHash(i + float3(1, 0, 0)), f.x),
                     lerp(VolHash(i + float3(0, 1, 0)), VolHash(i + float3(1, 1, 0)), f.x), f.y),
                lerp(lerp(VolHash(i + float3(0, 0, 1)), VolHash(i + float3(1, 0, 1)), f.x),
                     lerp(VolHash(i + float3(0, 1, 1)), VolHash(i + float3(1, 1, 1)), f.x), f.y), f.z);
}

// Henyey-Greenstein の位相関数を、g=0（全方向に同じ）で 1 になるよう 4π 倍したもの
float PhaseHG(float cosTheta, float g)
{
    float g2 = g * g;
    return (1.0f - g2) / pow(max(1.0f + g2 - 2.0f * g * cosTheta, 1e-4f), 1.5f);
}

// 点 p がスポットライトの円錐（前側）の中にあるか
bool InsideCone(float3 p, float3 apex, float3 axis, float cosOuter)
{
    float3 d = p - apex;
    float along = dot(d, axis);
    return along > 0.0f && along * along >= cosOuter * cosOuter * dot(d, d);
}

// 視線 c + t·v（t は [tMin, tMax]）と、頂点 apex・軸 axis・半角の cos が cosOuter の円錐（前側）が重なる区間を求める。
// 円錐の面 (d·axis)² = cos²·|d|² との交点を2次方程式で出し、区切られた3区間のうち中にあるものを選ぶ
bool IntersectCone(float3 c, float3 v, float3 apex, float3 axis, float cosOuter, float tMin, float tMax,
    out float t0, out float t1)
{
    t0 = tMin;
    t1 = tMax;
    float3 co = c - apex;
    float vd = dot(v, axis);
    float cd = dot(co, axis);
    float cos2 = cosOuter * cosOuter;
    float a = vd * vd - cos2;
    float b = 2.0f * (vd * cd - cos2 * dot(v, co));
    float cc = cd * cd - cos2 * dot(co, co);

    float r1 = tMax + 1.0f;
    float r2 = tMax + 1.0f;
    if (abs(a) < 1e-6f)
    {
        if (abs(b) > 1e-6f)
            r1 = r2 = -cc / b;
    }
    else
    {
        float disc = b * b - 4.0f * a * cc;
        if (disc >= 0.0f)
        {
            float sq = sqrt(disc);
            float ra = (-b - sq) / (2.0f * a);
            float rb = (-b + sq) / (2.0f * a);
            r1 = min(ra, rb);
            r2 = max(ra, rb);
        }
    }

    // 3区間 [tMin, r1] [r1, r2] [r2, tMax] のうち、中にある区間（ふつうは1つ）を選ぶ
    float bounds[4] = { tMin, clamp(r1, tMin, tMax), clamp(r2, tMin, tMax), tMax };
    bool found = false;
    [unroll]
    for (int k = 0; k < 3; ++k)
    {
        float s0 = bounds[k];
        float s1 = bounds[k + 1];
        if (s1 - s0 < 1e-3f)
            continue;
        if (InsideCone(c + v * (0.5f * (s0 + s1)), apex, axis, cosOuter))
        {
            if (!found)
            {
                t0 = s0;
                found = true;
            }
            t1 = s1;
        }
    }
    return found;
}

// スポットライトの光が空気中で散る量（光の筋）。pixel は画素位置（サンプルの位置を画素ごとにずらして縞を消す）
float3 CalcSpotVolumetric(float3 cameraPos, float3 dir, float L, float2 pixel)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    if (SpotVolumetricStrength <= 0.0f)
        return result;

    float jitter = frac(52.9829189f * frac(dot(pixel, float2(0.06711056f, 0.00583715f))));
    for (uint i = 0; i < LightCount; i++)
    {
        LightData light = Lights[i];
        if (!light.enabled || light.type != 2 || light.padding.x <= 0.0f)
            continue;

        float3 axis = normalize(light.direction);
        // まず光の届く距離の球で視線を切り、その中で円錐と重なる区間を求める
        float3 toLight = light.position - cameraPos;
        float b = dot(toLight, dir);
        float h2 = max(dot(toLight, toLight) - b * b, 0.0f);
        float r2 = light.distance * light.distance;
        if (h2 >= r2)
            continue;
        float halfChord = sqrt(r2 - h2);
        float tMin = max(b - halfChord, 0.0f);
        float tMax = min(b + halfChord, L);
        if (tMax <= tMin)
            continue;
        float t0;
        float t1;
        if (!IntersectCone(cameraPos, dir, light.position, axis, light.cosAngle, tMin, tMax, t0, t1))
            continue;

        float innerCos = light.padding.z > light.cosAngle ? light.padding.z : 1.0f;
        float steps = max(SpotVolumetricSteps, 1.0f);
        float stepLength = (t1 - t0) / steps;
        float3 sum = float3(0.0f, 0.0f, 0.0f);
        for (float s = 0.0f; s < steps; s += 1.0f)
        {
            float t = t0 + (s + jitter) * stepLength;
            float3 p = cameraPos + dir * t;
            float3 fromLight = p - light.position;
            float d = length(fromLight);
            float3 lightDir = fromLight / max(d, 1e-4f);
            float attenuation = 1.0f / (1.0f + light.decay * d * d);
            float angle = smoothstep(light.cosAngle, innerCos, dot(lightDir, axis));
            // 光源のすぐ近くは薄くして、光が上の暗がりから差してくるように見せる
            angle *= smoothstep(0.0f, max(SpotApexFade, 1e-3f), d);
            // 霧の濃淡。ゆっくり流して、光の筋の中で埃っぽい揺らぎを出す
            float noise = VolNoise(p * SpotFogNoiseScale + SpotFogNoiseSpeed * EnvTime);
            float density = max(1.0f + SpotFogNoiseInfluence * (noise * 2.0f - 1.0f), 0.0f);
            // 光の進む向きと、カメラへ向かう向きのなす角で、前方散乱を効かせる
            float phase = PhaseHG(dot(lightDir, -dir), SpotPhaseG);
            sum += attenuation * angle * density * phase;
        }
        result += light.color.rgb * light.intensity * light.padding.x * sum * stepLength;
    }
    return result * SpotVolumetricStrength;
}

// SSAO の結果（1 = 遮られていない）。SSAORenderPass が作る
Texture2D<float> gAO : register(t8);

// ポイントライトの光が、カメラから距離 L まで視線 dir に沿って空気中で散る量を足し合わせる。
// 減衰 1/(1 + decay·d²) = (1/decay)/(1/decay + d²) を線分で積分すると atan で閉じた形になる:
//   ∫ (1/decay)/(H² + (t-b)²) dt = (1/decay)/H · atan((t-b)/H)
//   （b = 光源の視線上への射影, H² = 光源と視線の距離² + 1/decay）
// 二乗の減衰は裾が長く、松明が何本もあると画面全体が霞むので、ライトの半径の球の中だけを積分し、
// 球の境目の減衰値 c を引いて境目で 0 になるようにする（霞が光源のまわりにまとまり、段も出ない）
float3 CalcVolumetricLight(float3 cameraPos, float3 dir, float L)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    if (VolumetricStrength <= 0.0f)
        return result;
    for (uint i = 0; i < LightCount; i++)
    {
        LightData light = Lights[i];
        if (!light.enabled || light.type != 1 || light.padding.x <= 0.0f)
            continue;
        float3 toLight = light.position - cameraPos;
        float b = dot(toLight, dir);
        float h2 = max(dot(toLight, toLight) - b * b, 0.0f);
        float r2 = light.radius * light.radius;
        if (h2 >= r2)
            continue;
        // 視線が球を通る区間を [0, L] で切る
        float halfChord = sqrt(r2 - h2);
        float t0 = max(b - halfChord, 0.0f);
        float t1 = min(b + halfChord, L);
        if (t1 <= t0)
            continue;
        float invDecay = 1.0f / max(light.decay, 1e-3f);
        float H = sqrt(h2 + invDecay);
        float edge = 1.0f / (1.0f + light.decay * r2);
        float integral = invDecay / H * (atan((t1 - b) / H) - atan((t0 - b) / H)) - edge * (t1 - t0);
        result += light.color.rgb * light.intensity * light.padding.x * max(integral, 0.0f);
    }
    return result * VolumetricStrength;
}

// 距離フォグの量（0〜1）。仕様書 3.1 の 1 - exp(-density * distance) に、掛け始める距離と上限を足したもの
float CalcDistanceFog(float3 cameraPos, float3 P)
{
    if (DistanceFogDensity <= 0.0f)
        return 0.0f;
    float d = max(length(P - cameraPos) - DistanceFogStart, 0.0f);
    return min(1.0f - exp(-DistanceFogDensity * d), DistanceFogMaxOpacity);
}

// 高さフォグ。濃さ ρ(y) = d·exp(k·(y - y0)) の霧を、カメラから P までの線分で積分した量を返す（0〜1）。
// ∫ρ ds = d·L·exp(k·(c.y - y0))·(exp(k·Δy) - 1)/(k·Δy)   （Δy = P.y - c.y, L = |P - c|）
float CalcHeightFog(float3 cameraPos, float3 P)
{
    if (HeightFogDensity <= 0.0f)
        return 0.0f;
    float L = length(P - cameraPos);
    float dy = P.y - cameraPos.y;
    float kdy = HeightFogFalloff * dy;
    // Δy がほぼ0（水平に見ている）なら (e^x - 1)/x → 1
    float grow = abs(kdy) > 1e-4f ? (exp(clamp(kdy, -30.0f, 30.0f)) - 1.0f) / kdy : 1.0f;
    float base = exp(clamp(HeightFogFalloff * (cameraPos.y - HeightFogBaseHeight), -30.0f, 30.0f));
    float amount = HeightFogDensity * L * base * grow;
    return min(1.0f - exp(-amount), HeightFogMaxOpacity);
}

// Normal decode
float3 DecodeNormal(float3 packed)
{
    return normalize(packed * 2.0f - 1.0f);
}

// ShadowMap (CSM 3カスケード)
Texture2D<float> gShadow0 : register(t5);
Texture2D<float> gShadow1 : register(t6);
Texture2D<float> gShadow2 : register(t7);

SamplerComparisonState sampler_Shadow : register(s1);

// CSM 定数バッファ (C++ の CascadeLightingCB と同一レイアウト)
cbuffer LightMatrixCB : register(b4)
{
    float4x4 LightViewProj0;   // cascade 0
    float4x4 LightViewProj1;   // cascade 1
    float4x4 LightViewProj2;   // cascade 2
    float4   CascadeFar;       // xyz = 各カスケードの view-space far 深度
    float4x4 CameraView;       // view-space Z 計算用
}

// 1枚のシャドウマップへの PCF 3x3 サンプリング
float SampleShadowPCF(Texture2D<float> shadowMap, float4x4 lvp, float3 worldPos)
{
    float4 shadowPos = mul(float4(worldPos, 1.0), lvp);
    shadowPos.xyz /= shadowPos.w;

    float2 uv;
    uv.x = shadowPos.x * 0.5 + 0.5;
    uv.y = -shadowPos.y * 0.5 + 0.5;

    float depth = shadowPos.z - 0.002f;

    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || depth < 0 || depth > 1)
        return 1.0;

    float shadow = 0.0;
    float2 texelSize = 1.0 / float2(1280.0, 1280.0);
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            shadow += shadowMap.SampleCmpLevelZero(
                sampler_Shadow,
                uv + float2(x, y) * texelSize,
                depth
            );
        }
    }
    return shadow / 9.0;
}

float CalcShadow(float3 worldPos)
{
    // カメラ view-space Z でカスケードを選択
    float viewZ = mul(float4(worldPos, 1.0), CameraView).z;

    if (viewZ < CascadeFar.x)
        return SampleShadowPCF(gShadow0, LightViewProj0, worldPos);
    if (viewZ < CascadeFar.y)
        return SampleShadowPCF(gShadow1, LightViewProj1, worldPos);
    if (viewZ < CascadeFar.z)
        return SampleShadowPCF(gShadow2, LightViewProj2, worldPos);

    return 1.0;
}