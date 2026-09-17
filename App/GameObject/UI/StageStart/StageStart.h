#pragma once
#include <Math/Vector3.h>

class StageStart
{
public:
	StageStart() = default;
	~StageStart() = default;
	// 初期化処理
	void Initialize();
	// 更新処理
	void Update();

	bool IsComplete() const { return isComplete_; }

private:
	void Complete();
	// GameCameraを引きの到達点（プレイヤーの背後）へ先に置く
	void PrepareGameCamera();

	bool isComplete_ = false;

	// プレイヤーの向き（水平・単位ベクトル）。
	// アップの位置と引きの到達点で同じ「背後」を使うため、Initializeで一度求めて保持する
	Vector3 playerForward_{ 0.0f, 0.0f, 1.0f };
};

