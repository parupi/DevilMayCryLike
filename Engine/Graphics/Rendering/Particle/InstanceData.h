#pragma once
#include <Math/Vector4.h>
#include <Math/Matrix4x4.h>

struct InstanceData {
    Matrix4x4 world;
    Matrix4x4 wvp;
    Vector4 color;
};