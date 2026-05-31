#include "HellkainaStateAttackA.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Player/Player.h"

HellkainaWeaponAttackState::HellkainaWeaponAttackState(HellkainaWeapon* weapon, WeaponMotionParams params)
    : weapon_(weapon), params_(std::move(params))
{
}

void HellkainaWeaponAttackState::Enter(Enemy& enemy)
{
    timer_ = 0.0f;
    enemy.SetIsAttack(true);
}

void HellkainaWeaponAttackState::Update(Enemy& enemy, float deltaTime)
{
    timer_ += deltaTime;
    float t = timer_ / params_.duration;

    Vector3 pos = CatmullRomSpline(params_.translate, t);
    Vector3 rot = CatmullRomSpline(params_.rotate,    t);

    weapon_->GetWorldTransform()->GetTranslation() = pos;
    weapon_->GetRenderer(weapon_->name_)->GetWorldTransform()->GetRotation() = EulerDegree(rot);

    if (params_.moveSpeed > 0.0f) {
        Vector3 dir = Normalize(
            enemy.GetPlayer()->GetWorldTransform()->GetTranslation() -
            enemy.GetWorldTransform()->GetTranslation());
        dir.y = 0.0f;
        enemy.GetWorldTransform()->GetTranslation() += dir * params_.moveSpeed * deltaTime;
    }

    if (timer_ >= params_.duration) {
        enemy.ChangeState(EnemyStateName::Idle);
    }
}

void HellkainaWeaponAttackState::Exit(Enemy& enemy)
{
    enemy.SetIsAttack(false);
}
