#include "HellkainaStateKnockBack.h"
#include <base/utility/DeltaTime.h>
#include "GameObject/Character/Enemy/Enemy.h"

void HellkainaStateKnockBack::Enter(const DamageInfo& info, Enemy& enemy)
{
    currentType_ = info.type;
    velocity_ = info.direction * info.impulseForce;
    stateTime_.current = 0.0f;

    // ★ 回転関連初期化
    currentTilt_ = 0.0f;
    targetTilt_ = 20.0f;

    switch (info.type)
    {
    case ReactionType::HitStun:
        stunTimer_ = info.stunTime;
        velocity_ *= 0.25f;
        tiltAmount_ = 15.0f;
        break;

    case ReactionType::Knockback:
        velocity_.y += info.impulseForce * info.upwardRatio;
        angularVel_ = info.torqueForce;
        enemy.GetWorldTransform()->GetTranslation().y += 0.3f;  // 10cmだけ上げる
        enemy.SetOnGround(false);
        break;

    case ReactionType::Launch:
        velocity_.y += info.impulseForce * info.upwardRatio * 1.4f;
        angularVel_ = info.torqueForce;
        break;
    }
}


void HellkainaStateKnockBack::Update(Enemy& enemy, float deltaTime)
{
    float dt = deltaTime;
    stateTime_.current += dt;

    velocity_.y += -9.8f * dt;
    enemy.SetVelocity(velocity_);

    if (currentType_ != ReactionType::HitStun) {
        float rotate = Lerp(0.0f, angularVel_, stateTime_.current);
        enemy.GetRenderer(enemy.name_)->GetWorldTransform()->GetRotation() = EulerDegree({ rotate, rotate, rotate });
    }

    if (currentType_ == ReactionType::HitStun) {
        // 徐々にtargetTilt_へ近づける（吹っ飛んでるときののけぞり）
        currentTilt_ = Lerp(currentTilt_, targetTilt_, dt * 5.0f);

        // X軸のみに回転を付ける（のけぞり感が最も出る）
        enemy.GetRenderer(enemy.name_)->GetWorldTransform()->GetRotation() = EulerDegree({ currentTilt_, 0.0f, 0.0f });
    }

    if (currentType_ == ReactionType::HitStun && (stunTimer_ -= dt) <= 0) {
        enemy.ChangeState("Idle");
        return;
    }

    if (enemy.GetOnGround()) {
        OnLand(enemy);
    }
}

void HellkainaStateKnockBack::OnLand(Enemy& enemy)
{
    if (currentType_ == ReactionType::Launch ||
        currentType_ == ReactionType::Knockback)
    {
        velocity_ *= 0.3f;
        enemy.GetRenderer(enemy.name_)->GetWorldTransform()->GetRotation() = { 0.0f, 0.0f, 0.0f };
        enemy.ChangeState("Idle");
        return;
    }
}


void HellkainaStateKnockBack::Exit(Enemy& enemy)
{
    enemy;
}
