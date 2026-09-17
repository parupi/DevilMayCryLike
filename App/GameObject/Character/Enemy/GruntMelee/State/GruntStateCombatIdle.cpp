#include "GruntStateCombatIdle.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include <cstdlib>
#include <ctime>

namespace
{
    constexpr float kCooldown = 0.8f;

    // 距離しきい値
    constexpr float kNearDist = 3.0f;
    constexpr float kMidDist  = 8.0f;

    // 近距離の行動確率 (0〜99)
    // 0〜39: AttackNormal, 40〜59: SideMove, 60〜79: Retreat, 80〜99: Approach
    constexpr int kNear_AttackNormal = 40;
    constexpr int kNear_SideMove     = 60;
    constexpr int kNear_Retreat      = 80;

    // 中距離の行動確率
    // 0〜29: RushAttack, 30〜59: Approach, 60〜79: SideMove, 80〜99: AttackNormal
    constexpr int kMid_RushAttack    = 30;
    constexpr int kMid_Approach      = 60;
    constexpr int kMid_SideMove      = 80;

    // 遠距離の行動確率
    // 0〜59: Approach, 60〜89: SideMove, 90〜99: Retreat
    constexpr int kFar_Approach  = 60;
    constexpr int kFar_SideMove  = 90;

    bool srandDone = false;
}

GruntStateCombatIdle::GruntStateCombatIdle(EnemySensorComponent* sensor)
    : sensor_(sensor)
{
}

void GruntStateCombatIdle::Enter(Enemy& enemy)
{
    enemy;
    if (!srandDone) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        srandDone = true;
    }
}

void GruntStateCombatIdle::Update(Enemy& enemy, float deltaTime)
{
    if (!enemy.GetOnGround()) {
        enemy.ChangeState(EnemyStateName::Air);
        return;
    }

    sensor_->Update(enemy);

    if (inCooldown_) {
        cooldownTimer_ += deltaTime;
        if (cooldownTimer_ < kCooldown) return;
        inCooldown_    = false;
        cooldownTimer_ = 0.0f;
    }

    float dist = sensor_->GetDistanceToPlayer();
    int   r    = std::rand() % 100;

    // 攻撃行動が許可されていない敵（チュートリアルの練習台など）は、
    // 攻撃の抽選結果を移動行動に置き換える
    const bool canAttack = enemy.CanAttack();

    if (dist <= kNearDist) {
        if (r < kNear_AttackNormal) {
            enemy.ChangeState(canAttack ? GruntMeleeStateName::AttackNormal
                                        : GruntMeleeStateName::SideMove);
        } else if (r < kNear_SideMove) {
            enemy.ChangeState(GruntMeleeStateName::SideMove);
        } else if (r < kNear_Retreat) {
            enemy.ChangeState(GruntMeleeStateName::Retreat);
        } else {
            enemy.ChangeState(GruntMeleeStateName::Approach);
        }
    } else if (dist <= kMidDist) {
        if (r < kMid_RushAttack) {
            enemy.ChangeState(canAttack ? GruntMeleeStateName::RushAttack
                                        : GruntMeleeStateName::Approach);
        } else if (r < kMid_Approach) {
            enemy.ChangeState(GruntMeleeStateName::Approach);
        } else if (r < kMid_SideMove) {
            enemy.ChangeState(GruntMeleeStateName::SideMove);
        } else {
            enemy.ChangeState(canAttack ? GruntMeleeStateName::AttackNormal
                                        : GruntMeleeStateName::SideMove);
        }
    } else {
        if (r < kFar_Approach) {
            enemy.ChangeState(GruntMeleeStateName::Approach);
        } else if (r < kFar_SideMove) {
            enemy.ChangeState(GruntMeleeStateName::SideMove);
        } else {
            enemy.ChangeState(GruntMeleeStateName::Retreat);
        }
    }

    inCooldown_ = true;
}

void GruntStateCombatIdle::Exit(Enemy& enemy)
{
    enemy;
}
