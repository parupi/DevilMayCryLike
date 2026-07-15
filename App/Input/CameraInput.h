#pragma once
#include "Math/Vector2.h"

class Input;

struct CameraInputContext {
	// スティックの方向
	Vector2 stickDirection;
	// スティックの傾きの大きさ
	float stickMagnitude;
};

// カメラに関する入力を受け渡す
class CameraInput
{
public:
	CameraInput() = default;
	~CameraInput() = default;

	// 初期化
	void Initialize(Input* input);
	// 入力の更新
	void Update();
	// スティックの方向を取得
	const Vector2& GetStickDirection() const { return context_.stickDirection; }
	// スティックの傾きの大きさを取得
	float GetStickMagnitude() const { return context_.stickMagnitude; }
private:
	Input* input_ = nullptr;

	CameraInputContext context_;
};

