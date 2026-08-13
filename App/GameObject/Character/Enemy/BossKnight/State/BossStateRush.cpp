#include "BossStateRush.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"

namespace {
    // 突進。体当たりでプレイヤーへ突っ込む。判定は体全体。
    // 吹き飛ばされた直後にこれへ移行するので、距離を一気に詰め直す役割も持つ
    BoneAttackParams MakeRushParams() {
        BoneAttackParams p;
        p.jointName = "BodyRoot";
        // ワールド単位。体当たりなので胴まわりを広めに取る
        p.halfExtents = { 1.5f, 1.1f, 1.5f };
        p.offset = { 0.0f, 0.0f, 0.0f };        // ジョイントの向きは骨ごとに違うので原点のまま使う
        p.duration = 0.88f;                     // Dragon_Attack のクリップ長
        p.rushSpeed = 22.0f;                    // 判定が出ている間だけ突っ込む

        p.damage.damage = 1.0f;
        p.damage.type = ReactionType::Knockback;
        p.damage.impulseForce = 18.0f;
        p.damage.upwardRatio = 0.35f;
        p.damage.stunTime = 0.7f;

        // 突進は判定が出ている間だけ前進するので、窓を長めに取って距離を稼ぐ。
        // 噛みつきと同じ Dragon_Attack を使うが、あちらのイベント（短い窓）に
        // 引きずられると距離を詰められないので、ここは比率で動かす
        p.preferEvents = false;
        p.hitStartRatio = 0.35f;
        p.hitEndRatio = 0.85f;
        return p;
    }
}

BossStateRush::BossStateRush(EnemyBoneAttackComponent* attack)
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

void BossStateRush::Exit(Enemy& enemy) { attack_->Cancel(enemy); }
