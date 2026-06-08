#include "GruntStateRushAttack.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMeleeAttackComponent.h"

namespace
{
    MeleeAttackParams MakeRushParams()
    {
        MeleeAttackParams p;
        p.windupDuration = 0.65f;   // 大きく引いて溜める時間
        p.attackDuration = 0.30f;   // 突進・振り抜く時間（短く・鋭く）
        p.rushSpeed      = 18.0f;   // 突進速度（攻撃フェーズのみ）

        // 構えポーズ: 武器を大きく後方・横に引く（突進前の溜め）
        // t=1: 突進しながら振り抜いた状態
        p.weaponTranslate = {
            { -1.0f,  0.8f,  0.4f },  // ghost
            { -1.0f,  0.8f,  0.4f },  // 構え: 武器を横・後方に大きく引く
            {  0.15f, 0.3f, -0.95f }, // 振り抜き中
            {  0.35f, -0.15f, -1.0f }, // ghost
        };
        p.weaponRotate = {
            {  15.0f,  40.0f,  65.0f },  // ghost
            {  15.0f,  40.0f,  65.0f },  // 構え: 武器を外向きに傾けて引く
            { -55.0f,   0.0f,  25.0f },  // 振り抜き中
            {-115.0f,   0.0f,  25.0f },  // ghost
        };
        return p;
    }
}

GruntStateRushAttack::GruntStateRushAttack(EnemyMeleeAttackComponent* attack)
    : attack_(attack)
{
}

void GruntStateRushAttack::Enter(Enemy& enemy)
{
    attack_->BeginAttack(enemy, MakeRushParams());
}

void GruntStateRushAttack::Update(Enemy& enemy, float deltaTime)
{
    attack_->Update(enemy, deltaTime);

    if (attack_->IsFinished()) {
        enemy.ChangeState(GruntMeleeStateName::CombatIdle);
    }
}

void GruntStateRushAttack::Exit(Enemy& enemy)
{
    enemy;
}
