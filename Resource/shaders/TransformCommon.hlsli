#ifndef TRANSFORM_COMMON
#define TRANSFORM_COMMON

cbuffer TransformCB : register(b1)
{
    float4x4 WVP; // World * View * Proj
    float4x4 World; // World space transform
    float4x4 WorldInverseTranspose; // For normals
};

#endif
