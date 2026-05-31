#include "Hellkaina.h"
#include "GameObject/Character/Player/Player.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Collider/AABBCollider.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include "../State/EnemyStateIdle.h"
#include "../State/EnemyStateAir.h"
#include "../State/EnemyStateMove.h"
#include "../State/EnemyStateKnockBack.h"
#include "../State/EnemyStateSideMove.h"
#include <scene/Transition/TransitionManager.h>
#include <3d/Collider/CollisionManager.h>
#include "base/Particle/ParticleManager.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include <GameObject/Character/Enemy/State/HellkainaStateAttackA.h>

namespace
{
    // AttackA のモーションデータ（近距離薙ぎ）
    WeaponMotionParams MakeAttackAParams()
    {
        WeaponMotionParams p;
        p.duration  = 0.25f;
        p.moveSpeed = 2.0f;
        p.translate = {
            { -0.75f,  0.75f, -0.75f },
            { -0.75f,  0.75f, -0.75f },
            {  0.0f,   0.35f, -0.85f },
            {  0.25f,  0.0f,  -1.0f  },
        };
        p.rotate = {
            {   0.0f, 0.0f, 30.0f },
            {   0.0f, 0.0f, 30.0f },
            { -45.0f, 0.0f, 30.0f },
            {-100.0f, 0.0f, 30.0f },
        };
        return p;
    }

    // AttackB のモーションデータ（高速突進）
    WeaponMotionParams MakeAttackBParams()
    {
        WeaponMotionParams p;
        p.duration  = 0.4f;
        p.moveSpeed = 16.0f;
        p.translate = {
            { -0.75f,  0.75f, -0.75f },
            { -0.75f,  0.75f, -0.75f },
            {  0.0f,   0.35f, -0.85f },
            {  0.25f,  0.0f,  -1.0f  },
        };
        p.rotate = {
            {   0.0f, 0.0f, 30.0f },
            {   0.0f, 0.0f, 30.0f },
            { -45.0f, 0.0f, 30.0f },
            {-100.0f, 0.0f, 30.0f },
        };
        return p;
    }
}

Hellkaina::Hellkaina(std::string objectName) : Enemy(objectName)
{
    RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_, "PlayerBody"));
    AddRenderer(RendererManager::GetInstance()->FindRender(name_));
    GetRenderer(name_)->GetWorldTransform()->GetScale() = { 0.8f, 0.8f, 0.8f };

    hp_ = 10.0f;
}

void Hellkaina::Initialize()
{
    // コライダーのサイズ調整
    auto* col = static_cast<AABBCollider*>(GetCollider(name_));
    col->GetColliderData().offsetMax *= 0.5f;
    col->GetColliderData().offsetMin *= 0.5f;

    // 武器の生成（ステート生成より先に行う）
    RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_ + "HellkainaWeapon", "Sword"));
    CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>(name_ + "HellkainaWeapon"));

    auto weapon = std::make_unique<HellkainaWeapon>(name_ + "HellkainaWeapon");
    weapon->AddRenderer(RendererManager::GetInstance()->FindRender(name_ + "HellkainaWeapon"));
    weapon->AddCollider(CollisionManager::GetInstance()->FindCollider(name_ + "HellkainaWeapon"));
    weapon->Initialize();
    weapon->GetWorldTransform()->SetParent(GetWorldTransform());

    weapon_ = weapon.get();
    Object3dManager::GetInstance()->AddObject(std::move(weapon));

    // ステートの登録（weapon_ が確定した後なので AttackA/B に渡せる）
    states_[EnemyStateName::Idle]      = std::make_unique<EnemyStateIdle>();
    states_[EnemyStateName::Move]      = std::make_unique<EnemyStateMove>();
    states_[EnemyStateName::Air]       = std::make_unique<EnemyStateAir>();
    states_[EnemyStateName::KnockBack]     = std::make_unique<EnemyStateKnockBack>();
    states_[HellkainaStateName::SideMove]  = std::make_unique<EnemyStateSideMove>();
    states_[HellkainaStateName::AttackA]  = std::make_unique<HellkainaWeaponAttackState>(weapon_, MakeAttackAParams());
    states_[HellkainaStateName::AttackB]  = std::make_unique<HellkainaWeaponAttackState>(weapon_, MakeAttackBParams());

    // パーティクル
    ParticleManager::GetInstance()->CreateEmitter(name_ + "HitEffect", "EnemyDamageEmitter");
    auto& emitters = ParticleManager::GetInstance()->GetEmitters();
    emitter_ = emitters.at(name_ + "HitEffect").get();
    emitter_->SetParent(GetWorldTransform());
    emitter_->AddParticle("EnemyDamageEffect");
    emitter_->AddParticle("PlayerSlashEffect");
    emitter_->SetActiveFlag(false);

    Enemy::Initialize();
}

void Hellkaina::Update(float deltaTime)
{
    weapon_->SetIsDraw(isAttack_);
    Enemy::Update(deltaTime);
}

#ifdef _DEBUG
void Hellkaina::DebugGui()
{
    ImGui::Begin(name_.c_str());
    Object3d::DebugGui();
    ImGui::End();
}
#endif // DEBUG

void Hellkaina::OnCollisionEnter(BaseCollider* other)
{
    Enemy::OnCollisionEnter(other);

    if (other->category_ != CollisionCategory::PlayerWeapon) return;
    if (!player_ || !player_->IsAttack()) return;

    // KnockBack 中は重複して被弾しない（ヒットストップとエフェクトは毎回発火）
    if (currentState_ == states_[EnemyStateName::KnockBack].get()) {
        hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
        emitter_->Emit();
        return;
    }

    hp_ -= player_->GetAttackData().damage;
    if (hp_ <= 0.0f) {
        OnDeath();
    }

    // DamageInfo を構築して Enemy に渡す（KnockBack ステートが Enter() で参照する）
    DamageInfo info;
    info.damage          = player_->GetAttackData().damage;
    info.hitPosition     = GetWorldTransform()->GetTranslation();
    info.attackerPosition = player_->GetWorldTransform()->GetTranslation();
    info.direction       = Normalize(info.hitPosition - info.attackerPosition);
    info.type            = player_->GetAttackData().type;
    info.impulseForce    = player_->GetAttackData().impulseForce;
    info.upwardRatio     = player_->GetAttackData().upwardRatio;
    info.torqueForce     = player_->GetAttackData().torqueForce;
    info.stunTime        = player_->GetAttackData().stunTime;

    SetPendingDamageInfo(info);
    ChangeState(EnemyStateName::KnockBack);

    hitStop_->Start(player_->GetAttackData().hitStopTime, player_->GetAttackData().hitStopIntensity * 3.0f);
    emitter_->Emit();
}

void Hellkaina::OnCollisionStay(BaseCollider* other)
{
    Enemy::OnCollisionStay(other);
}

void Hellkaina::OnCollisionExit(BaseCollider* other)
{
    Enemy::OnCollisionExit(other);
}
