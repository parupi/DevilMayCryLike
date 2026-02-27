#pragma once
#include <math/function.h>
#include <math/Vector4.h>
#include <math/Vector3.h>
#include <math/Vector2.h>

enum class FadeType {
	None = 0, // フェードしない
	Alpha = 1, // 透明度で消える
	ScaleShrink = 2, // 寿命末期で急速に縮む
};

struct Particle {
	EulerTransform transform;
	Vector3 velocity;
	Vector3 acc;
	Vector4 color;
	float lifeTime;
	float currentTime;
	bool isAlive;
	Vector3 initialScale; // 生成時のスケールを保持
	float shrinkStart; // 縮小開始時間 (0.0f - 1.0f)
	FadeType fadeType = FadeType::Alpha; // デフォルトをAlphaに
	bool isBillboard;
};
