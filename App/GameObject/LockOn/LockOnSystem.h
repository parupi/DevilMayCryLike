#pragma once
#include "LockOnTarget.h"
#include <vector>

// ロックオン対象の選定をするクラス
class LockOnSystem
{
public:
	LockOnSystem() = default;
	~LockOnSystem() = default;

	// ロックオン対象を追加
	void RegisterTarget(LockOnTarget* target);
	// ロックオン対象を削除
	void UnregisterTarget(LockOnTarget* target);

	// ロックオンの更新
	void Update();
	// 現在のターゲットを取得
	LockOnTarget* GetCurrentTarget() const;

private:
	std::vector<LockOnTarget*> targets_;
	LockOnTarget* currentTarget_ = nullptr;
};

