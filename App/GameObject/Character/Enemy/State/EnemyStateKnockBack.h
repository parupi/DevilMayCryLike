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
