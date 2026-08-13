#include "BossStateHeavySword.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"

namespace {
    // 叩きつけ。翼と体を使って前方を薙ぎ払う、遅くて重い攻撃。
    // 予備動作が長いぶん見てから避けられるが、当たると大きく吹き飛ぶ
    BoneAttackParams MakeSlamParams() {
        BoneAttackParams p;
        p.jointName = "BodyRoot";               // 体ごと叩きつけるので体幹に付ける
        // ワールド単位。翼幅を含む、避けにくい広範囲
        p.halfExtents = { 2.2f, 1.2f, 1.8f };
        p.offset = { 0.0f, 0.0f, 0.0f };        // ジョイントの向きは骨ごとに違うので原点のまま使う
        p.duration = 1.67f;                     // Dragon_Attack2 のクリップ長
        p.rushSpeed = 0.0f;                     // その場で叩きつける

        p.damage.damage = 2.0f;                 // 噛みつきの倍
        p.damage.type = ReactionType::Knockback;
        p.damage.impulseForce = 22.0f;
        p.damage.upwardRatio = 0.5f;
        p.damage.stunTime = 0.9f;

        // フォールバック。Dragon_Attack2 は翼の角速度ピークが 1.083/1.67 秒＝65%
        p.hitStartRatio = 0.58f;
        p.hitEndRatio = 0.75f;
        return p;
    }
}

BossStateHeavySword::BossStateHeavySword(EnemyBoneAttackComponent* attack)
    : attack_(attack) {}

void BossStateHeavySword::Enter(Enemy& enemy)
{
    attack_->BeginAttack(enemy, MakeSlamParams());
}

void BossStateHeavySword::Update(Enemy& enemy, float deltaTime)
{
    attack_->Update(enemy, deltaTime);
    if (attack_->IsFinished()) {
        enemy.ChangeState(BossStateName::CombatIdle);
    }
}

void BossStateHeavySword::Exit(Enemy& enemy) { attack_->Cancel(enemy); }
