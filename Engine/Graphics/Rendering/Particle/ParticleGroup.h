#pragma once
#include <vector>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Particle.h"
#include "World3D/Object/Renderer/PrimitiveType.h"

// 発生位置オフセットに対する放射方向の速度モード
enum class RadialMode {
	None = 0,     // 通常（min/maxVelocity のランダム速度）
	Converge = 1, // 収束: 寿命が尽きる瞬間に発生中心へ到達する速度を与える
	Diverge = 2,  // 拡散: 発生中心から外向きに RadialSpeed で飛ばす
};

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
	int radialMode = 0;          // RadialMode
	float radialSpeed = 1.0f;    // Converge: 速度倍率 / Diverge: 外向き速度(m/s)
};

struct ParticleGroup
{
	std::vector<Particle> particles;
	ParticleParameters params;
	PrimitiveType shape = PrimitiveType::Plane;
};