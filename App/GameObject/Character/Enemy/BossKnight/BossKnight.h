#pragma once
#include "GameObject/Character/Enemy/Enemy.h"
#include "BossWeapon.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMeleeAttackComponent.h"
#include "base/Particle/ParticleEmitter.h"

/// <summary>
/// ボスエネミー：BossKnight
///
/// 通常敵の約3倍のHPを持ち、HP割合に応じて3フェーズに変化する。
/// Slash（速い縦斬り）・HeavySword（遅い叩きつけ）・Rush（高速突進）の
/// 3種類の攻撃を状況と残りHPに応じて使い分ける。
/// </summary>
class BossKnight : public Enemy
{
public:
    static constexpr float kMaxHp = 25.0f;

    BossKnight(std::string objectName);
    void Initialize() override;
    void Update(float deltaTime) override;

#ifdef _DEBUG
    void DebugGui() override;
#endif

    void OnCollisionEnter(BaseCollider* other) override;
    void OnCollisionStay(BaseCollider* other) override;
    void OnCollisionExit(BaseCollider* other) override;

private:
    BossWeapon*      weapon_        = nullptr;
    ParticleEmitter* hitEmitter_    = nullptr;
    ParticleEmitter* chargeEmitter_ = nullptr;

    float chargeEmitTimer_ = 0.0f;
    static constexpr float kChargeEmitInterval = 0.08f;

    // ── ヒット蓄積ノックバック ──────────────────────────────────────
    // kKnockbackThreshold 分のダメージが溜まると初めて吹き飛ぶ
    float hitAccumulation_           = 0.0f;
    static constexpr float kKnockbackThreshold = 3.0f; // 通常攻撃約3発分

    std::unique_ptr<EnemySensorComponent>      sensor_;
    std::unique_ptr<EnemyMovementComponent>    movement_;
    std::unique_ptr<EnemyMeleeAttackComponent> meleeAttack_;
};
