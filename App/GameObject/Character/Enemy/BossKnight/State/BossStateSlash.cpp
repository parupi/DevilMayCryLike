#include "BossStateSlash.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "GameObject/Character/Enemy/BossKnight/BossKnight.h"

namespace {
	// 噛みつき。頭に小さめの判定を出す速い攻撃。
	// リーチが短いぶんダメージも軽く、連発できる位置取りへの牽制に使う
	BoneAttackParams MakeBiteParams() {
		BoneAttackParams p;
		p.jointName = "Head";
		// スケール1のときのワールド単位（BeginAttack が配置スケールを掛ける）。
		// プレイヤーのコライダーは1辺1.0で、ドラゴンを2倍で置くと頭は地上3m前後まで上がる。
		// **プレイヤーの背丈は敵の大きさに関係なく1mのまま**なので、頭からの判定は
		// 「足元まで垂れ下がる」くらい縦を取らないと頭上を素通りする。
		// 箱の向きはジョイントの向きに従う（頭の骨は首の方向を向く）ため、
		// どの軸が下を向いても届くよう3軸とも大きめに取っている
		p.halfExtents = { 1.1f, 1.1f, 1.1f };
		p.offset = { 0.0f, 0.0f, 0.0f }; // ジョイントの向きは骨ごとに違うので原点のまま使う
		p.duration = 0.88f;                   // Dragon_Attack のクリップ長
		p.rushSpeed = 3.0f;                   // 噛みつきながら少し踏み込む

		p.damage.damage = 1.0f;
		p.damage.type = ReactionType::Knockback;
		p.damage.impulseForce = 12.0f;
		p.damage.upwardRatio = 0.25f;
		p.damage.stunTime = 0.5f;

		// .anim.json にイベントが無い場合のフォールバック。
		// Dragon_Attack は BodyRoot の角速度ピークが 0.583/0.88 秒＝67% なのでその前後
		p.hitStartRatio = 0.55f;
		p.hitEndRatio = 0.78f;
		return p;
	}
}

BossStateSlash::BossStateSlash(EnemyBoneAttackComponent* attack)
	: attack_(attack) {}

void BossStateSlash::Enter(Enemy& enemy) {
	attack_->BeginAttack(enemy, MakeBiteParams());
}

void BossStateSlash::Update(Enemy& enemy, float deltaTime) {
	attack_->Update(enemy, deltaTime);
	if (attack_->IsFinished()) {
		enemy.ChangeState(BossStateName::CombatIdle);
	}
}

void BossStateSlash::Exit(Enemy& enemy) { attack_->Cancel(enemy); }
