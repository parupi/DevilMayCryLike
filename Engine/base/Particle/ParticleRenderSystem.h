#pragma once
#include <vector>
#include "Particle.h"
#include "InstanceData.h"

class BaseCamera;

class ParticleRenderSystem
{
public:
    void BuildInstances(const std::vector<Particle>& particles, BaseCamera* camera, std::vector<InstanceData>& outInstances);
};