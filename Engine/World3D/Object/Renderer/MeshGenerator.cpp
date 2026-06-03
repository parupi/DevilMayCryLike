// MeshGenerator.cpp
#include "MeshGenerator.h"
#include <cmath>
#include <DirectXMath.h>

MeshData MeshGenerator::CreatePlane(float width, float height) {
    MeshData mesh;
    float w = width * 0.5f;
    float h = height * 0.5f;

    mesh.vertices = {
        {{-w, 0, -h, 1}, {0, 1}, {0, 1, 0}},
        {{-w, 0,  h, 1}, {0, 0}, {0, 1, 0}},
        {{ w, 0, -h, 1}, {1, 1}, {0, 1, 0}},
        {{ w, 0,  h, 1}, {1, 0}, {0, 1, 0}},
    };

    mesh.indices = {
        0, 1, 2,
        2, 1, 3
    };

    return mesh;
}

MeshData MeshGenerator::CreateRing(float outerRadius, float innerRadius, int sliceCount) {
    MeshData mesh;
    for (int i = 0; i <= sliceCount; ++i) {
        float theta = float(i) / sliceCount * DirectX::XM_2PI;
        float cosT = std::cos(theta);
        float sinT = std::sin(theta);
        Vector3 normal = { 0, 1, 0 };

        mesh.vertices.push_back({ {innerRadius * cosT, 0, innerRadius * sinT, 1}, {float(i) / sliceCount, 1}, normal });
        mesh.vertices.push_back({ {outerRadius * cosT, 0, outerRadius * sinT, 1}, {float(i) / sliceCount, 0}, normal });
    }

    for (int i = 0; i < sliceCount; ++i) {
        int i0 = i * 2;
        int i1 = i0 + 1;
        int i2 = i0 + 2;
        int i3 = i2 + 1;

        mesh.indices.insert(mesh.indices.end(), {
            i0, i1, i2,
            i2, i1, i3
            });
    }

    return mesh;
}

MeshData MeshGenerator::CreateCylinder(float radius, float height, int sliceCount, int stackCount) {
    MeshData mesh;
    float halfHeight = height * 0.5f;

    for (int stack = 0; stack <= stackCount; ++stack) {
        float y = -halfHeight + (height * stack / stackCount);

        for (int slice = 0; slice <= sliceCount; ++slice) {
            float theta = float(slice) / sliceCount * DirectX::XM_2PI;
            float x = std::cos(theta);
            float z = std::sin(theta);
            Vector3 normal = { x, 0, z };
            Vector2 uv = { float(slice) / sliceCount, float(stack) / stackCount };

            mesh.vertices.push_back({
                {radius * x, y, radius * z, 1},
                uv,
                normal
                });
        }
    }

    int ringCount = sliceCount + 1;
    for (int stack = 0; stack < stackCount; ++stack) {
        for (int slice = 0; slice < sliceCount; ++slice) {
            int i0 = stack * ringCount + slice;
            int i1 = i0 + 1;
            int i2 = i0 + ringCount;
            int i3 = i2 + 1;

            mesh.indices.insert(mesh.indices.end(), {
                i0, i2, i1,
                i1, i2, i3
                });
        }
    }

    return mesh;
}
