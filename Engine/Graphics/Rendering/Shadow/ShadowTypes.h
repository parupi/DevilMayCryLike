#pragma once
#include <cstdint>
#include <Math/Matrix4x4.h>

static constexpr uint32_t kCascadeCount = 3;

struct CascadeData
{
    Matrix4x4 lightViewProj;
    float splitDepth;   // View空間Z
};