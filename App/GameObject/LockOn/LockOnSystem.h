#pragma once
#include "LockOnTarget.h"
#include <vector>

class LockOnInput;
class Player;
class Sprite;

// ロックオン対象の選定をするクラス
class LockOnSystem
{
public:
	LockOnSystem() = default;
	~LockOnSystem() = default;

	// 初期化
	void Initialize(LockOnInput* input, Player* player);
	// ロックオンの更新
	void Update();

	// ロックオン対象を追加
	void RegisterTarget(LockOnTarget* target);
	// ロックオン対象を削除
	void UnregisterTarget(LockOnTarget* target);

	// 現在のターゲットを取得
	LockOnTarget* GetCurrentTarget() const { return currentTarget_; }
	// ターゲットがいるかどうかを確認
	bool IsLockOn() { return currentTarget_ != nullptr; }
private:
	// ロックオンの入力を判別するクラス
	LockOnInput* input_ = nullptr;
	// プレイヤーの参照
	Player* player_ = nullptr;
	// レティクル描画用のスプライト
	Sprite* reticle_ = nullptr;

	std::vector<LockOnTarget*> targets_;
	LockOnTarget* currentTarget_ = nullptr;
	// 最適なターゲットを探す
	LockOnTarget* FindBestTarget();
	// 各ターゲットのスコア計算
	float CalculateScore(LockOnTarget* target);
};

