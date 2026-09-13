#pragma once
#include "World3D/Object/Object3d.h"
#include "Math/Vector3.h"
#include "GameObject/Character/CharacterStructs.h"
#include "../BaseState/EnemyStateBase.h"

class Enemy;

/// <summary>
/// 敵の被弾リアクション（仕様書 §11）。
///
/// **速度はこのステートが持たない**。ノックバックの速度・減衰・時間は Enemy が持つ
/// KnockbackComponent の担当で、ここは「リアクションの見た目」と
/// 「いつ通常の行動へ戻るか」だけを見る。
///
/// 分けてあるおかげで、多段ヒットで何度当たってもステートを入れ直す必要がなく
/// （傾きや回転が巻き戻らない）、のけぞらない敵でも同じ速度計算が使える。
/// </summary>
class EnemyStateKnockBack : public EnemyStateBase {
public:
	EnemyStateKnockBack() = default;
	~EnemyStateKnockBack() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;

	// 被弾リアクション。行動を止められていても最後まで再生する
	bool IsReaction() const override { return true; }

protected:
	// ノックバック／のけぞり終了後に遷移するステート名。
	// 既定は Idle。派生クラスで上書きして別のステート（例: ボスの Rush）へ繋げられる。
	virtual const char* NextState() const;

private:
	// のけぞりの傾き（度）。吹き飛び・打ち上げは KnockbackData::torque の回転を使う
	static constexpr float kStunTiltDegree = 15.0f;
	static constexpr float kBlowTiltDegree = 20.0f;
	static constexpr float kTiltFollowRate = 5.0f;

	static float TiltFor(ReactionType type);

	float currentTilt_ = 0.0f;
	float targetTilt_ = 0.0f;
};
