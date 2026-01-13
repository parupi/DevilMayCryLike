#pragma once
#include <cstdint>
#include <math/Matrix4x4.h>

static constexpr uint32_t kCascadeCount = 3;

struct CascadeData
{
    float splitDepth; // View空間Z
    Matrix4x4 lightViewProj;
};