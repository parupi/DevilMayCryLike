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
    /// 見た目のモデル。Resource/models/Enemys/Skeleton/Skeleton.obj を指す
    static constexpr const char* kModelName = "Enemys/Skeleton";
    /// 素の高さ約5m を約1.2m にするスケール。大きさを変えるならここ
    static constexpr float kModelScale = 0.24f;

    // Skeleton.gltf が持つクリップ（5種）。差し替えは Initialize の RegisterStateClip と合わせて見ること
    static constexpr const char* kClipIdle   = "Skeleton_Idle";
    static constexpr const char* kClipRun    = "Skeleton_Running";
    static constexpr const char* kClipAttack = "Skeleton_Attack";
    static constexpr const char* kClipDeath  = "Skeleton_Death";
    static constexpr const char* kClipSpawn  = "Skeleton_Spawn";
    /// Skeleton_Attack(0.93秒)で振り切る瞬間の位置。Torso の角速度ピークが 0.567秒＝61%
    static constexpr float kAttackImpactRatio = 0.61f;

    GruntMelee(std::string objectName);
    void Initialize() override;
    void Update(float deltaTime) override;

#ifdef _DEBUG
#endif

    void OnCollisionEnter(BaseCollider* other) override;
    void OnCollisionStay(BaseCollider* other) override;
    void OnCollisionExit(BaseCollider* other) override;

protected:
    /// <summary>死亡演出終了時に武器を後始末する</summary>
    void OnDeathEffectFinished() override;

private:
    GruntMeleeWeapon* weapon_        = nullptr;
    ParticleEmitter*  chargeEmitter_ = nullptr;

    float chargeEmitTimer_ = 0.0f;
    static constexpr float kChargeEmitInterval = 0.1f;

    std::unique_ptr<EnemySensorComponent>      sensor_;
    std::unique_ptr<EnemyMovementComponent>    movement_;
    std::unique_ptr<EnemyMeleeAttackComponent> meleeAttack_;
};
