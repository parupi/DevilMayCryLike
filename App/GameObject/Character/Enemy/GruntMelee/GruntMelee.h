#pragma once
#include "GameObject/Character/Enemy/Enemy.h"
#include "GruntMeleeWeapon.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMeleeAttackComponent.h"
#include "Graphics/Rendering/Particle/ParticleEmitter.h"

/// <summary>
/// 近接攻撃型の敵。
/// - State が意思決定（待機・接近・左右移動・後退・通常攻撃・突進攻撃）
/// - Component が行動実行（感知・移動・武器モーション）
/// </summary>
class GruntMelee : public Enemy
{
public:
    GruntMelee(std::string objectName);
    void Initialize() override;
    void Update(float deltaTime) override;

#ifdef _DEBUG
    void DebugGui() override;
#endif

    void OnCollisionEnter(BaseCollider* other) override;
    void OnCollisionStay(BaseCollider* other) override;
    void OnCollisionExit(BaseCollider* other) override;

private:
    GruntMeleeWeapon* weapon_        = nullptr;
    ParticleEmitter*  emitter_       = nullptr;
    ParticleEmitter*  chargeEmitter_ = nullptr;

    float chargeEmitTimer_ = 0.0f;
    static constexpr float kChargeEmitInterval = 0.1f;

    std::unique_ptr<EnemySensorComponent>      sensor_;
    std::unique_ptr<EnemyMovementComponent>    movement_;
    std::unique_ptr<EnemyMeleeAttackComponent> meleeAttack_;
};
