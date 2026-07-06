#include "BossStateHeavySword.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyMeleeAttackComponent.h"

namespace
{
    MeleeAttackParams MakeHeavyParams()
    {
        MeleeAttackParams p;
        p.windupDuration = 0.9f;   // 長い溜め（プレイヤーに予備動作が見える）
        p.attackDuration = 0.35f;  // ゆっくりだが強力な叩きつけ
        p.rushSpeed      = 0.0f;   // その場で叩きつける

        // 両手で高く持ち上げてから叩きつける
        p.weaponTranslate = {
            { -0.3f,  2.2f,  0.1f },
            { -0.3f,  2.2f,  0.1f },
            {  0.0f, -0.6f, -0.9f },
            {  0.1f, -0.9f, -0.9f },
        };
        p.weaponRotate = {
            {  80.0f,   0.0f,  80.0f },
            {  80.0f,   0.0f,  80.0f },
            { -100.0f,  0.0f,  10.0f },
            { -140.0f,  0.0f,  10.0f },
        };
        return p;
    }
}

BossStateHeavySword::BossStateHeavySword(EnemyMeleeAttackComponent* attack)
    : attack_(attack) {}

void BossStateHeavySword::Enter(Enemy& enemy)
{
    attack_->BeginAttack(enemy, MakeHeavyParams());
}

void BossStateHeavySword::Update(Enemy& enemy, float deltaTime)
{
    attack_->Update(enemy, deltaTime);
    if (attack_->IsFinished()) {
        enemy.ChangeState(BossStateName::CombatIdle);
    }
}

void BossStateHeavySword::Exit(Enemy&) {}
