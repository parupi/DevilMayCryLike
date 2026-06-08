#include "BossStateRush.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMeleeAttackComponent.h"

namespace
{
    MeleeAttackParams MakeRushParams()
    {
        MeleeAttackParams p;
        p.windupDuration = 0.6f;
        p.attackDuration = 0.3f;
        p.rushSpeed      = 22.0f;  // GruntMelee(18)より速い突進

        // 武器を後方に引いて突進しながら薙ぎ払う
        p.weaponTranslate = {
            { -1.1f,  0.9f,  0.5f },
            { -1.1f,  0.9f,  0.5f },
            {  0.2f,  0.4f, -1.1f },
            {  0.4f,  0.1f, -1.1f },
        };
        p.weaponRotate = {
            {  20.0f,  45.0f,  70.0f },
            {  20.0f,  45.0f,  70.0f },
            { -65.0f,   0.0f,  20.0f },
            {-120.0f,   0.0f,  20.0f },
        };
        return p;
    }
}

BossStateRush::BossStateRush(EnemyMeleeAttackComponent* attack)
    : attack_(attack) {}

void BossStateRush::Enter(Enemy& enemy)
{
    attack_->BeginAttack(enemy, MakeRushParams());
}

void BossStateRush::Update(Enemy& enemy, float deltaTime)
{
    attack_->Update(enemy, deltaTime);
    if (attack_->IsFinished()) {
        enemy.ChangeState(BossStateName::CombatIdle);
    }
}

void BossStateRush::Exit(Enemy& enemy) {}
