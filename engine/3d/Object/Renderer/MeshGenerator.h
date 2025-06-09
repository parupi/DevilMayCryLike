#pragma once
#include "Model/ModelStructs.h"

class MeshGenerator {
public:
    static MeshData CreatePlane(float width = 1.0f, float height = 1.0f);
    static MeshData CreateRing(float outerRadius = 1.0f, float innerRadius = 0.5f, int sliceCount = 32);
    static MeshData CreateCylinder(float radius = 0.5f, float height = 1.0f, int sliceCount = 32, int stackCount = 1);
};