#pragma once
#include <math/Vector4.h>
#include <math/Matrix4x4.h>

struct InstanceData {
    Matrix4x4 world;
    Matrix4x4 wvp;
    Vector4 color;
};