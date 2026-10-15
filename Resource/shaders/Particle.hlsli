struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    // 粒のワールド座標（ソフトパーティクルが地面との距離を測るのに使う）
    float3 worldPos : TEXCOORD1;
    // x = 粒ごとの乱数 / y = 寿命の進み具合
    float4 misc : TEXCOORD2;
    // スプライトシートで切り出す前のUV。ノイズはコマに関係なく板全体に貼る
    float2 meshTexcoord : TEXCOORD3;
    // 粒の中心が光源から受けた光（光源に照らされるグループだけ。頂点シェーダーが計算する）
    float3 lighting : TEXCOORD4;
};

// グループ単位の設定。ParticleManager.h の ParticleGroupConstantsGPU と並び順を合わせること
cbuffer ParticleGroupParam : register(b1)
{
    float time;
    float noiseDistortion;
    float noiseColorBlend;
    float emissiveIntensity;

    float2 noiseTiling;
    float2 noiseScroll;

    float noiseErosion;
    float noiseErosionSoftness;
    float softDistance;
    uint flags;

    float3 cameraPosition;
    float _pad0;

    // カメラからの距離で薄くする（kFlagCameraFade）
    float nearFadeStart;
    float nearFadeEnd;
    float farFadeStart;
    float farFadeEnd;

    // 光源に照らされる（kFlagLit）
    float3 ambientDustColor; // 光が当たっていない所での明るさ
    float ambientVisibility; // 光が当たっていない所での透明度の倍率
    float lightResponse;     // 光の強さで透明度がどれだけ上がるか
    float colorResponse;     // 光源の色をどれだけ映すか（0=明るさだけ）
    float alphaScale;        // グループ全体に掛ける透明度の倍率
    float debugView;         // 2=埃の受けた光だけ / 3=埃の透明度（RenderEnvironment::DebugView）
};

// ParticleShaderFlag と合わせること
static const uint kFlagNoise = 1;
static const uint kFlagGrayscale = 2;
static const uint kFlagSoft = 4;
static const uint kFlagLit = 8;
static const uint kFlagCameraFade = 16;
