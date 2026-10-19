#include "GBuffer.hlsli"

// 静的なモデル（床・壁・岩）をまとめて描くための GBuffer.VS。
// オブジェクトごとの定数バッファ（TransformCommon の WVP）の代わりに、
// インスタンスごとの行列を StructuredBuffer から読み、カメラの VP はルート定数で受け取る。
// ピクセルシェーダーは GBuffer.PS をそのまま使う（StaticInstanceBatcher）

struct InstanceTransform
{
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

cbuffer InstanceCamera : register(b1)
{
    float4x4 ViewProj;
};

StructuredBuffer<InstanceTransform> gInstances : register(t2);

VSOutput main(VertexInput input, uint instanceId : SV_InstanceID)
{
    VSOutput output;
    InstanceTransform instance = gInstances[instanceId];

    float4 worldPos = mul(input.position, instance.World);
    output.worldPosWS = worldPos;
    output.positionCS = mul(worldPos, ViewProj);

    float3 worldNormal = normalize(mul(float4(input.normal.xyz, 0.0f), instance.WorldInverseTranspose).xyz);
    output.normalWS = float4(worldNormal, 1.0f);

    output.uv_and_pad = float4(input.uv, 0.0f, 0.0f);
    return output;
}
