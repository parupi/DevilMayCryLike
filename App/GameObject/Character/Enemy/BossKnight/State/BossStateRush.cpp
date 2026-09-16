#include "BossStateRush.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "Audio/GameSoundLibrary.h"
#include "Audio/SoundManager.h"

namespace {
    // 突進。体当たりで正面へまっすぐ突っ込む。判定は体全体。
    // 吹き飛ばされた直後にこれへ移行するので、距離を一気に詰め直す役割も持つ
    BoneAttackParams MakeRushParams() {
        BoneAttackParams p;
        p.jointName = "BodyRoot";
        // スケール1のときのワールド単位（BeginAttack が配置スケールを掛ける）。
        // 体当たりなので胴まわりを広めに、縦は足元のプレイヤーまで届く長さを取る
        p.halfExtents = { 1.7f, 1.4f, 1.7f };
        p.offset = { 0.0f, 0.0f, 0.0f };        // ジョイントの向きは骨ごとに違うので原点のまま使う
        p.duration = 0.88f;                     // Dragon_Attack のクリップ長
        p.rushSpeed = 22.0f;                    // 判定が出ている間だけ、振り始めの正面へ突っ込む

        p.damage.damage = 1.0f;
        p.damage.knockback.type = ReactionType::Knockback;
        p.damage.knockback.power = 18.0f;
        p.damage.knockback.verticalPower = 18.0f * 0.35f;
        p.damage.knockback.stunTime = 0.7f;

        // 突進は判定が出ている間だけ前進するので、窓を長めに取って距離を稼ぐ。
        // 噛みつきと同じ Dragon_Attack を使うが、あちらのイベント（短い窓）に
        // 引きずられると距離を詰められないので、ここは比率で動かす
        p.preferEvents = false;
        p.hitStartRatio = 0.35f;
        p.hitEndRatio = 0.85f;

        // 溜めを 0.31秒 → 約0.9秒 に伸ばす。突進は距離を一気に詰めてくるぶん、
        // 踏み込む前にその場で構える時間を長めに取って進路から逃げられるようにする
        p.extraWindupTime = 0.6f;

        // 溜めの間の向き直り[度/秒]。遅くして、横へ走れば帯から抜けられるようにする
        // （8m先を走るプレイヤーは約70度/秒で回り込む）
        p.windupTurnSpeed = 45.0f;

        // 予兆。突進の進路をそのまま帯で示す。体はこの帯の上をなぞって走る。
        // 帯の手前の端（forwardOffset）が体の後ろ端で、体の前端が奥の端へ届いたところで止まる。
        // 長さ = 体の厚み 3.4（判定の halfExtents.z の2倍）+ 進む距離 4.8
        //      （配置2倍の実寸で 9.6m ≒ 22m/s × 判定が出ている 0.44秒）。
        // 奥の端は体当たりの判定が届く所とちょうど同じ（足元から実寸13m先）
        // （スケール1基準。ステージ配置が2倍なので実寸は幅3.8m・長さ16.4m）
        p.telegraph.shape = TelegraphShape::Rect;
        p.telegraph.halfWidth = 1.9f;
        p.telegraph.length = 8.2f;
        p.telegraph.forwardOffset = -1.7f;

        // 踏み込みの音だけここで指定する。
        // 突進中のループと止まったときの衝撃は、長さを自分で持つ必要があるので BossStateRush 側
        p.windupSound = GameSound::kDragonRushStep;
        p.soundVolume = 0.95f;
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
    const bool wasRushing = attack_->IsHitActive();

    attack_->Update(enemy, deltaTime);

    // 走っている間だけ風を切る音を鳴らし続ける。
    // 判定が出ている間 = 前へ進んでいる間なので、突進の見た目とぴったり合う
    const bool isRushing = attack_->IsHitActive();
    if (isRushing && rushVoice_ < 0) {
        SEPlayParams loop;
        loop.name = GameSound::kDragonRushLoop;
        loop.volume = 0.8f;
        loop.loop = true;
        rushVoice_ = SoundManager::GetInstance().PlaySE(loop);
    } else if (!isRushing && wasRushing) {
        // 止まった瞬間。ループを切って、踏ん張った衝撃を出す
        StopRushLoop();
        SoundManager::GetInstance().PlaySE3D(
            GameSound::kDragonRushStop, enemy.GetWorldTransform()->GetTranslation(), 0.95f);
    }

    if (attack_->IsFinished()) {
        enemy.ChangeState(BossStateName::CombatIdle);
    }
}

void BossStateRush::Exit(Enemy& enemy)
{
    // 被弾やブレイクで突進が中断されてもループを残さない
    StopRushLoop();
    attack_->Cancel(enemy);
}

void BossStateRush::StopRushLoop()
{
    if (rushVoice_ < 0) { return; }
    SoundManager::GetInstance().StopSE(rushVoice_);
    rushVoice_ = -1;
}
