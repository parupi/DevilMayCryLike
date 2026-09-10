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
        // スケール1のときのワールド単位（BeginAttack が配置スケールを掛ける）。
        // 翼幅を含む、避けにくい広範囲。体幹は宙に浮いているので、
        // 縦は足元のプレイヤー（高さ1m）まで垂れ下がる長さを取る
        p.halfExtents = { 2.4f, 1.5f, 2.0f };
        p.offset = { 0.0f, 0.0f, 0.0f };        // ジョイントの向きは骨ごとに違うので原点のまま使う
        p.duration = 1.67f;                     // Dragon_Attack2 のクリップ長
        p.rushSpeed = 0.0f;                     // その場で叩きつける

        p.damage.damage = 2.0f;                 // 噛みつきの倍
        p.damage.type = ReactionType::Knockback;
        p.damage.impulseForce = 22.0f;
        p.damage.upwardRatio = 0.5f;
        p.damage.stunTime = 0.9f;

        // フォールバック。Dragon_Attack2 は翼の角速度ピークが 1.083/1.67 秒＝65%。
        // この比率は溜めと本編の境目にも使うので、hit_start(0.97秒＝58%) に合わせてある
        p.hitStartRatio = 0.58f;
        p.hitEndRatio = 0.75f;

        // 溜めを 0.97秒 → 約1.8秒 に伸ばす。一番痛い大振りなので一番長く構える
        p.extraWindupTime = 0.85f;

        // 予兆。判定が体幹まわりの大きな箱（横±4.8m・前後±4.0m）なので、
        // 正面だけでなく後ろにも当たる。円で「体のまわり全部が危ない」と見せる
        // （スケール1基準。ステージ配置が2倍なので実寸は半径4.8m）
        p.telegraph.shape = TelegraphShape::Circle;
        p.telegraph.radius = 2.4f;
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
