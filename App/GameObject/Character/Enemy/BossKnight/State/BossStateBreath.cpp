#include "BossStateBreath.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "GameObject/Character/Enemy/BossKnight/BossKnight.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"

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
        p.damage.type = ReactionType::Knockback;
        p.damage.impulseForce = 20.0f;
        p.damage.upwardRatio = 0.35f;
        p.damage.stunTime = 1.0f;

        // Dragon_Attack2 のイベントは叩きつけ用の短い窓なので、そのままだと
        // 炎が一瞬しか出ない。ここは比率で長い窓を作る
        p.preferEvents = false;
        p.hitStartRatio = 0.35f;  // 0.98秒。ここまでがクリップ側の溜め
        p.hitEndRatio = 0.93f;    // 判定（＝炎）は約1.6秒続く

        // 溜めは通常攻撃の3種より更に長い。合計2.4秒あるので必殺技だと分かる
        p.extraWindupTime = 1.4f;

        // 予兆。判定（offset.z ± halfExtents.z）と同じ帯をそのまま地面に描く。
        // 塗りが口元から奥へ走るので、炎がどこまで届くかが吐く前に分かる
        // （スケール1基準。ステージ配置が2倍なので実寸は幅3.5m・長さ11m）
        p.telegraph.shape = TelegraphShape::Rect;
        p.telegraph.halfWidth = 0.875f;
        p.telegraph.length = 5.5f;
        p.telegraph.forwardOffset = 0.25f;
        return p;
    }

    // 炎を吐いている間の向き直りの速さ[度/秒]。
    // これを超えて回れないので、横へ走られると照準が置いていかれる＝避けられる
    constexpr float kBreathTurnSpeed = 42.0f;

    // 炎VFXの発生間隔[s]。細かく出すほど炎が途切れずに繋がる
    constexpr float kFlameInterval = 0.03f;

    // 口元の位置（スケール1のときのワールド単位）。判定の手前側の端に合わせてある
    constexpr float kMouthHeight = 0.75f;
    constexpr float kMouthForward = 0.6f;
}

BossStateBreath::BossStateBreath(EnemyBoneAttackComponent* attack)
    : attack_(attack) {}

void BossStateBreath::Enter(Enemy& enemy)
{
    attack_->BeginAttack(enemy, MakeBreathParams());
    emitTimer_ = 0.0f;
    // 溜めの間はまだ普通に向き直る（＝プレイヤーへ狙いを付ける）。
    // 制限を掛けるのは炎が出てから（Update 側）
}

void BossStateBreath::Update(Enemy& enemy, float deltaTime)
{
    const bool wasFiring = attack_->IsHitActive();
    attack_->Update(enemy, deltaTime);

    if (attack_->IsHitActive()) {
        // 吐き始めた瞬間に照準を固める。以降はゆっくりしか追ってこない
        if (!wasFiring) {
            enemy.SetFaceTurnSpeed(kBreathTurnSpeed);
        }

        emitTimer_ += deltaTime;
        while (emitTimer_ >= kFlameInterval) {
            EmitFlame(enemy);
            emitTimer_ -= kFlameInterval;
        }
    }

    if (attack_->IsFinished()) {
        enemy.ChangeState(BossStateName::CombatIdle);
    }
}

void BossStateBreath::Exit(Enemy& enemy)
{
    attack_->Cancel(enemy);
    // 途中で中断された場合も含め、向き直りの制限は必ず戻す
    enemy.SetFaceTurnSpeed(0.0f);
}

void BossStateBreath::EmitFlame(Enemy& enemy)
{
    // プレイヤーの方向ではなく **体が今向いている方向** へ吐く。
    // でないと向き直りを遅くした意味がなくなり、炎だけがプレイヤーを追ってしまう。
    // 敵はローカル -Z がプレイヤー側を向く（Enemy::Update の回転）
    const Matrix4x4& world = enemy.GetWorldTransform()->GetMatWorld();
    Vector3 forward = TransformNormal({ 0.0f, 0.0f, -1.0f }, world);
    forward.y = 0.0f;
    if (Length(forward) < 0.001f) return;
    forward = Normalize(forward);

    const Vector3 scale = enemy.GetWorldTransform()->GetWorldScale();
    const Vector3 mouth = enemy.GetWorldTransform()->GetWorldPos()
        + Vector3{ 0.0f, kMouthHeight * scale.y, 0.0f }
        + forward * (kMouthForward * scale.z);

    ParticleManager::GetInstance().PlayVFX(BossKnight::kBreathVfxName, mouth, forward);
}
