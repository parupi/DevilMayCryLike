#include "BossStateBreath.h"
#include <algorithm>
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "GameObject/Character/Enemy/BossKnight/BossBreathEffect.h"

namespace {
    // 火炎ブレス。口から前方へまっすぐ伸びる炎の帯。
    // 判定は体の正面基準なので、頭の骨のローカル軸に振り回されない
    BoneAttackParams MakeBreathParams() {
        BoneAttackParams p;
        p.orientToBody = true;                  // jointName は使わない

        // スケール1のときのワールド単位（BeginAttack が配置スケールを掛ける）。
        // ステージ配置が2倍なので、実寸は幅3.5m・高さ3m・長さ11m の帯になる
        p.halfExtents = { 0.875f, 0.75f, 2.75f };
        // 体の正面基準（+Z がプレイヤー側・+Y が上）。
        // 口元の高さから前方へ、足元のプレイヤーまで届く帯を張る
        p.offset = { 0.0f, 0.75f, 3.0f };

        // Dragon_Attack2 のクリップ長(1.67秒)より長い。
        // 足りない分は EnemyBoneAttackComponent がクリップを引き伸ばして流す
        p.duration = 2.8f;
        p.rushSpeed = 0.0f;                     // その場で吐き続ける

        p.damage.damage = 3.0f;                 // 叩きつけ(2.0)より更に重い必殺技
        p.damage.knockback.type = ReactionType::Knockback;
        p.damage.knockback.power = 20.0f;
        p.damage.knockback.verticalPower = 20.0f * 0.35f;
        p.damage.knockback.stunTime = 1.0f;

        // Dragon_Attack2 のイベントは叩きつけ用の短い窓なので、そのままだと
        // 炎が一瞬しか出ない。ここは比率で長い窓を作る
        p.preferEvents = false;
        p.hitStartRatio = 0.35f;  // 0.98秒。ここまでがクリップ側の溜め
        p.hitEndRatio = 0.93f;    // 判定（＝炎）は約1.6秒続く

        // 溜めは通常攻撃の3種より更に長い。合計2.4秒あるので必殺技だと分かる
        p.extraWindupTime = 1.4f;

        // 溜めの間の向き直り[度/秒]。溜めが一番長いぶん一番遅くして、
        // 帯の外へ走り込めば避けられるようにする
        p.windupTurnSpeed = 35.0f;

        // 予兆。判定（offset.z ± halfExtents.z）と同じ帯をそのまま地面に描く。
        // 塗りが口元から奥へ走るので、炎がどこまで届くかが吐く前に分かる。
        // 吐き始めた瞬間に体の向きがここで固定されるので、炎も判定もこの帯の上をなぞる
        // （スケール1基準。ステージ配置が2倍なので実寸は幅3.5m・長さ11m）
        p.telegraph.shape = TelegraphShape::Rect;
        p.telegraph.halfWidth = 0.875f;
        p.telegraph.length = 5.5f;
        p.telegraph.forwardOffset = 0.25f;
        return p;
    }
}

AttackTelegraphParams BossStateBreath::GetTelegraph() { return MakeBreathParams().telegraph; }

BossStateBreath::BossStateBreath(EnemyBoneAttackComponent* attack, BossBreathEffect* effect)
    : attack_(attack), effect_(effect) {}

void BossStateBreath::Enter(Enemy& enemy)
{
    attack_->BeginAttack(enemy, MakeBreathParams());
    fireTimer_ = 0.0f;
    // 溜めの間はゆっくり向き直る（windupTurnSpeed）＝プレイヤーへ狙いを付けていく。
    // 吐き始めた瞬間に EnemyBoneAttackComponent が予兆の向きで体を固定する
}

void BossStateBreath::Update(Enemy& enemy, float deltaTime)
{
    attack_->Update(enemy, deltaTime);

    if (effect_) {
        if (attack_->IsWindingUp()) {
            effect_->RequestCharge(attack_->GetWindupProgress());
        } else if (attack_->IsHitActive()) {
            // 炎の帯の大きさは予兆と同じ（体の位置から奥の端まで）。配置スケールを掛けて実寸にする
            static const BoneAttackParams kParams = MakeBreathParams();
            const Vector3 scale = enemy.GetWorldTransform()->GetWorldScale();
            AttackTelegraphParams band = kParams.telegraph;
            band.ApplyScale(scale.x, scale.z);

            const float fireDuration = kParams.duration * (kParams.hitEndRatio - kParams.hitStartRatio);
            fireTimer_ += deltaTime;
            effect_->RequestFire(std::clamp(fireTimer_ / fireDuration, 0.0f, 1.0f),
                band.forwardOffset + band.length, band.halfWidth);
        } else {
            // 吐き終わって構えを解いている間。演出は余韻へ移る
            effect_->RequestStop();
        }
    }

    if (attack_->IsFinished()) {
        enemy.ChangeState(BossStateName::CombatIdle);
    }
}

void BossStateBreath::Exit(Enemy& enemy)
{
    // 途中で中断された場合も含め、予兆と体の向きの固定を後始末する
    attack_->Cancel(enemy);
    // 崩れなどで途中で抜けても、炎を出しっぱなしにしない
    if (effect_) {
        effect_->RequestStop();
    }
}
