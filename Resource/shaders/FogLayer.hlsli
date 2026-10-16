// 上部の霧の層（UpperFogRenderer）。UpperFogRenderer.h の FogLayerCB と並びを合わせること
cbuffer FogLayerCB : register(b0)
{
    float4x4 gViewProj;
    float3 gCenter;      // 板の中心（y が高さ）
    float gHalfSize;     // 板の半分の大きさ[m]
    float3 gColor;       // リニア
    float gOpacity;      // 一番濃い所の不透明度
    float3 gCameraPos;
    float gTime;
    float2 gNoiseOffset; // 流れた距離
    float gNoiseScale;   // 1m あたりの周期
    float gSoftDistance; // 壁や岩と交わる所を薄くする距離[m]
    float gEdgeFade;     // 板の端を薄くする幅（半分の大きさに対する割合）
    float gLayerSeed;
    float gGrazingFade;  // 1 なら視線が水平に近いほど薄くする（見上げる上部の霧）
    float _pad;
};

struct FogVSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 local : TEXCOORD1; // 板の中での位置（-1〜1）
};
