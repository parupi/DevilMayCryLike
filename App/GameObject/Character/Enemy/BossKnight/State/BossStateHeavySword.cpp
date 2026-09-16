#include "BossStateHeavySword.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "Audio/GameSoundLibrary.h"

namespace {
    // 叩きつけ。翼と体を使って前方を薙ぎ払う、遅くて重い攻撃。
    // 予備動作が長いぶん見てから避けられるが、当たると大きく吹き飛ぶ
    BoneAttackParams MakeSlamParams() {
        BoneAttackParams p;
        p.jointName = "BodyRoot";               // 体ごと叩きつけるので体幹に付ける
        // スケール1のときのワールド単位（BeginAttack が配置スケールを掛ける）。
        // 横と前後は予兆の円（半径2.4）に内接する正方形にしてある（半辺 2.4/√2 ≒ 1.7）。
        // 内接する正方形はどう回っても円からはみ出さないので、骨の向きに関係なく
        // 「赤い円の外にいれば当たらない」が守られる。
        // 体幹は宙に浮いているので、縦は足元のプレイヤー（高さ1m）まで垂れ下がる長さを取る
        p.halfExtents = { 1.7f, 1.5f, 1.7f };
        p.offset = { 0.0f, 0.0f, 0.0f };        // ジョイントの向きは骨ごとに違うので原点のまま使う
        p.duration = 1.67f;                     // Dragon_Attack2 のクリップ長
        p.rushSpeed = 0.0f;                     // その場で叩きつける

        p.damage.damage = 2.0f;                 // 噛みつきの倍
        p.damage.knockback.type = ReactionType::Knockback;
        p.damage.knockback.power = 22.0f;
        p.damage.knockback.verticalPower = 22.0f * 0.5f;
        p.damage.knockback.stunTime = 0.9f;

        // Dragon_Attack2 の中身（関節の高さを解析した結果）:
        //   0.28秒で頭が一番高く振りかぶり、0.32〜0.52秒で頭と翼を前下へ叩きつける（頭が一番低いのは0.40秒）。
        //   0.56秒から頭を戻し、0.97〜1.25秒はもう一度羽ばたいているだけ。
        // 以前は判定をこの2回目の羽ばたき(0.97〜1.25秒)に合わせていたので、
        // 「頭を叩きつけた少し後に判定が出る」ずれ方をしていた。叩きつけの窓へ移した。
        // この比率は溜めと本編の境目にも使うので、Dragon.anim.json の hit_start(0.32秒) と揃えてある
        p.hitStartRatio = 0.19f;   // 0.317秒
        p.hitEndRatio = 0.31f;     // 0.52秒

        // 溜め（振りかぶり）を 0.32秒 → 約1.8秒 に伸ばす。一番痛い大振りなので一番長く構える
        // （伸ばす前と同じ溜めの長さ。振りかぶりをゆっくり見せて、叩きつけは等速で鋭く出す）
        p.extraWindupTime = 1.5f;

        // 予兆。判定が体幹まわりの箱（配置2倍で横・前後とも±3.4m）なので、
        // 正面だけでなく後ろにも当たる。円で「体のまわり全部が危ない」と見せる
        // （スケール1基準。ステージ配置が2倍なので実寸は半径4.8m）。
        // どちらを向いても範囲は同じなので、溜めの間の向き直りは遅くしない
        p.telegraph.shape = TelegraphShape::Circle;
        p.telegraph.radius = 2.4f;

        // 振り上げている間の地鳴りと、叩きつけた瞬間の衝撃
        p.windupSound = GameSound::kDragonSlamCharge;
        p.strikeSound = GameSound::kDragonSlamImpact;
        p.soundVolume = 1.0f;
        return p;
    }
}

AttackTelegraphParams BossStateHeavySword::GetTelegraph() { return MakeSlamParams().telegraph; }

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
