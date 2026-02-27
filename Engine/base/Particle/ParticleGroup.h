#pragma once
#include <vector>
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "Particle.h"

struct ParticleParameters {
	Vector2 translateX;
	Vector2 translateY;
	Vector2 translateZ;
	Vector2 rotateX;
	Vector2 rotateY;
	Vector2 rotateZ;
	Vector2 scaleX;
	Vector2 scaleY;
	Vector2 scaleZ;
	Vector2 velocityX;
	Vector2 velocityY;
	Vector2 velocityZ;
	Vector2 lifeTime;
	Vector3 colorMin;
	Vector3 colorMax;
};

struct ParticleGroup
{
	std::vector<Particle> particles;
	ParticleParameters params;
};