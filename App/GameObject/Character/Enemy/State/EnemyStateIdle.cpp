#include "EnemyStateIdle.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Player/Player.h"
#include <cstdlib>
#include <ctime>

namespace
{
    constexpr float kNearDistance  = 3.0f;
    constexpr float kMidDistance   = 16.0f;
    constexpr float kAttackCooldown = 1.0f;

    // 0〜99 の乱数に対する累積しきい値
    constexpr int kNear_AttackA  = 10; //  0〜 9 : AttackA
    constexpr int kNear_SideMove = 20; // 10〜19 : SideMove / 残り: Move

    constexpr int kMid_AttackB   =  5; //  0〜 4 : AttackB
    constexpr int kMid_SideMove  = 30; //  5〜29 : SideMove / 残り: Move

    constexpr int kFar_SideMove  = 20; //  0〜19 : SideMove / 残り: Move

    bool srandInitialized = false;
}

void EnemyStateIdle::Enter(Enemy& enemy)
{
    enemy;
}

void EnemyStateIdle::Update(Enemy& enemy, float deltaTime)
{
    if (!enemy.GetOnGround()) {
        enemy.ChangeState(EnemyStateName::Air);
        return;
    }

    Player* player = enemy.GetPlayer();
    if (!player) return;

    if (!srandInitialized) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        srandInitialized = true;
    }

    if (isAttackCooldown_) {
        cooldownTimer_ += deltaTime;
        if (cooldownTimer_ >= kAttackCooldown) {
            cooldownTimer_ = 0.0f;
            isAttackCooldown_ = false;
        }
    }

    float distance = Length(
        player->GetWorldTransform()->GetTranslation() -
        enemy.GetWorldTransform()->GetTranslation());

    int r = std::rand() % 100;

    if (distance <= kNearDistance) {
        if (!isAttackCooldown_ && r < kNear_AttackA) {
            enemy.ChangeState(HellkainaStateName::AttackA);
            isAttackCooldown_ = true;
            cooldownTimer_ = 0.0f;
            return;
        }
        if (r < kNear_SideMove) {
            enemy.ChangeState(HellkainaStateName::SideMove);
            return;
        }
        enemy.ChangeState(EnemyStateName::Move);
        return;
    }

    if (distance <= kMidDistance) {
        if (!isAttackCooldown_ && r < kMid_AttackB) {
            enemy.ChangeState(HellkainaStateName::AttackB);
            isAttackCooldown_ = true;
            cooldownTimer_ = 0.0f;
            return;
        }
        if (r < kMid_SideMove) {
            enemy.ChangeState(HellkainaStateName::SideMove);
            return;
        }
        enemy.ChangeState(EnemyStateName::Move);
        return;
    }

    if (r < kFar_SideMove) {
        enemy.ChangeState(HellkainaStateName::SideMove);
    } else {
        enemy.ChangeState(EnemyStateName::Move);
    }
}

void EnemyStateIdle::Exit(Enemy& enemy)
{
    enemy;
}
