#include "EnemyStateIdle.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"
#include <cstdlib>
#include <ctime>

void EnemyStateIdle::Enter(Enemy& enemy)
{
    enemy;
}

void EnemyStateIdle::Update(Enemy& enemy)
{
    if (!enemy.GetOnGround()) {
        enemy.ChangeState("Air");
        return;
    }

    Player* player = enemy.GetPlayer();
    if (!player) {
        return;
    }

    // -------------------------
    // 乱数初期化（1回だけ）
    // -------------------------
    static bool initialized = false;
    if (!initialized) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        initialized = true;
    }

    // -------------------------
    // 攻撃クールタイム管理（ごり押し）
    // -------------------------
    static bool isAttackCooldown = false;
    static float cooldownTimer = 0.0f;
    constexpr float kAttackCooldownTime = 1.0f;

    if (isAttackCooldown) {
        cooldownTimer += DeltaTime::GetDeltaTime();
        if (cooldownTimer >= kAttackCooldownTime) {
            cooldownTimer = 0.0f;
            isAttackCooldown = false;
        }
    }

    // -------------------------
    // 距離計算
    // -------------------------
    float distance =
        Length(player->GetWorldTransform()->GetTranslation() -
            enemy.GetWorldTransform()->GetTranslation());

    constexpr float kAttackADistance = 3.0f;
    constexpr float kAttackBDistance = 16.0f;

    static LastAttack lastAttack = LastAttack::None;

    // -------------------------
    // 行動抽選
    // -------------------------
    int r = std::rand() % 100; // 0～99

    // -------------------------
    // 超近距離
    // -------------------------
    if (distance <= kAttackADistance) {

        // 攻撃可能な場合のみ抽選
        //if (!isAttackCooldown && lastAttack != LastAttack::AttackA) {
            // 0～59 : AttackA
            if (r < 10) {
                enemy.ChangeState("AttackA");
                lastAttack = LastAttack::AttackA;
                isAttackCooldown = true;
                cooldownTimer = 0.0f;
                return;
            }
        //}

        // 60～84 : 横移動
        if (r < 20) {
            enemy.ChangeState("SideMove");
            return;
        }

        // 85～99 : 前進
        enemy.ChangeState("Move");
        return;
    }

    // -------------------------
    // 中距離（突進圏内）
    // -------------------------
    if (distance <= kAttackBDistance) {

        //if (!isAttackCooldown && lastAttack != LastAttack::AttackB) {
            // 0～49 : AttackB
            if (r < 5) {
                enemy.ChangeState("AttackB");
                lastAttack = LastAttack::AttackB;
                isAttackCooldown = true;
                cooldownTimer = 0.0f;
                return;
            }
        //}

        // 50～79 : 横移動
        if (r < 30) {
            enemy.ChangeState("SideMove");
            return;
        }

        // 80～99 : 前進
        enemy.ChangeState("Move");
        return;
    }

    // -------------------------
    // 遠距離
    // -------------------------
    if (r < 20) {
        enemy.ChangeState("SideMove");
    } else {
        enemy.ChangeState("Move");
    }
}

void EnemyStateIdle::Exit(Enemy& enemy)
{
    enemy;
}
