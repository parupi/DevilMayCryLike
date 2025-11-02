// TransformCommon.hlsli

#ifndef TRANSFORM_COMMON
#define TRANSFORM_COMMON

cbuffer TransformCB : register(b1) // b1 = World/Camera用(自分のengineルールで変更してOK)
{
    float4x4 World; // local -> world
    float4x4 View; // world -> view
    float4x4 Proj; // view  -> clip
    float4x4 ViewProj; // view * proj (最適化で用意してもいい)
    float4 CameraPosWS; // camera position WorldSpace (ライティングで使う)
};

#endif
