// 静的なモデル（床・壁・岩）の影をまとめて描くための CascadedShadowMap.VS。
// オブジェクトごとの定数バッファの代わりに、インスタンスごとの行列を StructuredBuffer から読む
// （StaticInstanceBatcher。行列の並びは GBufferInstanced.VS と共通）

cbuffer LightVP : register(b1)
{
    float4x4 lightViewProj;
};

struct InstanceTransform
{
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

StructuredBuffer<InstanceTransform> gInstances : register(t0);

struct VSInput
{
    float3 pos : POSITION;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
};

VSOutput main(VSInput input, uint instanceId : SV_InstanceID)
{
    VSOutput o;
    float4 worldPos = mul(float4(input.pos, 1.0f), gInstances[instanceId].World);
    o.pos = mul(worldPos, lightViewProj);
    return o;
}
