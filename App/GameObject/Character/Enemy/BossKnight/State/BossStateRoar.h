#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMovementComponent;

/// <summary>
/// フェーズが変わった瞬間の咆哮。その場で一度吼えて、衝撃波とカメラの揺れで
/// 「ここから攻め方が変わる」を知らせる。
///
/// 吼えている間は攻撃を受け付けない（BossKnight::OnCollisionEnter が弾く）。
/// 短い間だけなので、プレイヤーは一歩引いて仕切り直す間として使える。
/// 体の発光は BossKnight::UpdateBodyVisual が GetElapsed() を見て合わせる。
///
/// 被弾リアクション（IsReaction）扱いにして、トレーニングの棒立ちでも最後まで流す
/// （途中で止まると、攻撃を受け付けないまま固まる）。
/// </summary>
class BossStateRoar : public EnemyStateBase
{
public:
    /// 息を吸ってから吼えるまで[s]。衝撃波と発光のピークはこの時刻
    static constexpr float kBurstTime = 0.4f;
    /// 咆哮全体の長さ[s]
    static constexpr float kDuration = 1.6f;

    explicit BossStateRoar(EnemyMovementComponent* movement);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;
    bool IsReaction() const override { return true; }

    /// <summary>咆哮を始めてからの経過[s]</summary>
    float GetElapsed() const { return timer_; }

private:
    // 吼えた瞬間の衝撃波・揺れ・光
    void Burst(Enemy& enemy);

    EnemyMovementComponent* movement_;
    float timer_ = 0.0f;
    bool burst_ = false;

    // 専用のクリップが無いので、羽ばたき（待機）をこの倍率で速く回して力んで見せる
    static constexpr float kWingFlapSpeed = 2.2f;
    static constexpr float kShakeTrauma = 0.6f;
};
