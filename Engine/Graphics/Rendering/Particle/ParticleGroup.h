#pragma once
#include <vector>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Particle.h"
#include "World3D/Object/Renderer/PrimitiveType.h"

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
	bool isBillboard;
};

struct ParticleGroup
{
	std::vector<Particle> particles;
	ParticleParameters params;
	PrimitiveType shape = PrimitiveType::Plane;
};