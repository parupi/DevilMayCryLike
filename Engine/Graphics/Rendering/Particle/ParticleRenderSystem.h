#pragma once
#include <vector>
#include "Particle.h"
#include "ParticleGroup.h"
#include "InstanceData.h"

class BaseCamera;

class ParticleRenderSystem
{
public:
    // グループ内のパーティクルから描画用インスタンスを組み立てる
    // （形状ごとに向き付けの基準軸が違うため、グループごと受け取る）
    void BuildInstances(const ParticleGroup& group, BaseCamera* camera, std::vector<InstanceData>& outInstances);
};
