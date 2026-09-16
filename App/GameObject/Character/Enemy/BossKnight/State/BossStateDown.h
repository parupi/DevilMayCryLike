#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"

class EnemyMovementComponent;

/// <summary>
/// ダメージが溜まって崩れている状態（ブレイク）。
///
/// のけぞらないボスに「攻め続けた見返り」を作るためのもの。
/// 溜まり具合と崩す判断は BossKnight（UpdateBreak）が持ち、このステートは崩れている間だけを受け持つ。
/// 崩れている間は動かず・振り向かず・被ダメージが増える（倍率は BossKnight::kDownDamageScale）。
///
/// 被弾リアクション（IsReaction）扱いにして、トレーニングの棒立ちでも最後まで流す。
/// </summary>
class BossStateDown : public EnemyStateBase
{
public:
    /// 崩れている長さ[s]
    static constexpr float kDuration = 3.0f;

    explicit BossStateDown(EnemyMovementComponent* movement);
    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;
    bool IsReaction() const override { return true; }

private:
    EnemyMovementComponent* movement_;
    float timer_ = 0.0f;
};
