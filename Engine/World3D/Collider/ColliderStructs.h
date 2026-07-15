#pragma once
#include <Math/Vector3.h>

struct AABBData {
	Vector3 offsetMax = { 0.5f, 0.5f, 0.5f };
	Vector3 offsetMin = { -0.5f, -0.5f, -0.5f };
	bool isActive = true;
};

struct SphereData {
	float radius = 0.5f;
	Vector3 offset = { 0.0f, 0.0f, 0.0f };
	bool isActive = true;
};

struct OBBData {
	Vector3 halfExtents = { 0.5f, 0.5f, 0.5f };
	Vector3 offset = { 0.0f, 0.0f, 0.0f };
	bool isActive = true;
};

// コライダー同士のめり込み排斥（MTV: 最小移動ベクトル）の結果
struct PenetrationResult {
	bool hit = false;
	Vector3 normal{}; // mover側を押し出す方向の単位ベクトル
	float depth = 0.0f;
};