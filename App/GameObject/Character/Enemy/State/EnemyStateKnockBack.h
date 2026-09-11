#pragma once
#include "World3D/Object/Object3d.h"
#include "Math/Vector3.h"
#include "GameObject/Character/CharacterStructs.h"
#include "../BaseState/EnemyStateBase.h"

class Enemy;

class EnemyStateKnockBack : public EnemyStateBase {
public:
	EnemyStateKnockBack() = default;
	~EnemyStateKnockBack() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

	// 被弾リアクション。行動を止められていても最後まで再生する
	bool IsReaction() const override { return true; }

	// 吹き飛び・打ち上げで宙に浮いている最中か（のけぞりは地上なので false）
	bool IsAirborne() const { return currentType_ != ReactionType::HitStun; }

	// 宙に浮いている最中に「のけぞり」の攻撃を受けたときの処理。
	// 状態は変えずに落下を止め、攻撃の ImpulseForce × UpwardRatio のぶんだけ持ち上げる（空中コンボで敵を空中に留める）
	void OnAirHit(Enemy& enemy, const DamageInfo& info);

protected:
	// ノックバック／のけぞり終了後に遷移するステート名。
	// 既定は Idle。派生クラスで上書きして別のステート（例: ボスの Rush）へ繋げられる。
	virtual const char* NextState() const;

private:
	void OnLand(Enemy& enemy);

	TimeData stateTime_;
	ReactionType currentType_;
	Vector3 velocity_;

	float stunTimer_ = 0.0f;
	float angularVel_ = 0.0f;

	float currentTilt_ = 0.0f;
	float targetTilt_ = 0.0f;
};
