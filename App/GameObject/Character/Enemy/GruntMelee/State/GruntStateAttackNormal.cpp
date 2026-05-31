#include "GruntStateAttackNormal.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMeleeAttackComponent.h"

namespace
{
    MeleeAttackParams MakeNormalParams()
    {
        MeleeAttackParams p;
        p.windupDuration = 0.55f;  // 構えを見せる時間
        p.attackDuration = 0.22f;  // 振り下ろしの時間（短く・鋭く）
        p.rushSpeed      = 0.0f;

        // t=0 が構えポーズ（頭上・後方に引いた状態）
        // t=1 が振り切った状態
        p.weaponTranslate = {
            { -0.8f,  1.4f,  0.1f },  // ghost（構えの少し先）
            { -0.8f,  1.4f,  0.1f },  // 構え: 武器を頭上・後方に大きく引く
            {  0.2f,  0.1f, -1.05f }, // 振り切り
            {  0.3f, -0.2f, -1.0f  }, // ghost（振り切り後）
        };
        p.weaponRotate = {
            {  50.0f, -25.0f,  85.0f },  // ghost
            {  50.0f, -25.0f,  85.0f },  // 構え: 武器を上に向けて引く
            { -80.0f,   0.0f,  20.0f },  // 振り切り
            {-125.0f,   0.0f,  20.0f },  // ghost
        };
        return p;
    }
}

GruntStateAttackNormal::GruntStateAttackNormal(EnemyMeleeAttackComponent* attack)
    : attack_(attack)
{
}

void GruntStateAttackNormal::Enter(Enemy& enemy)
{
    attack_->BeginAttack(enemy, MakeNormalParams());
}

void GruntStateAttackNormal::Update(Enemy& enemy, float deltaTime)
{
    attack_->Update(enemy, deltaTime);

    if (attack_->IsFinished()) {
        enemy.ChangeState(GruntMeleeStateName::CombatIdle);
    }
}

void GruntStateAttackNormal::Exit(Enemy& enemy)
{
    enemy;
}
