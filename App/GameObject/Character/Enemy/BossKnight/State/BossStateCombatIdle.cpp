#include "BossStateCombatIdle.h"
#include "BossStateSlash.h"
#include "BossStateHeavySword.h"
#include "BossStateBreath.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include <cstdlib>

namespace {
	// 抽選する行動の並び（kActionWeights の列）
	enum BossAction : int {
		kSlash,      // 噛みつき
		kHeavySword, // 叩きつけ
		kRush,       // 突進
		kApproach,   // 接近
		kActionCount
	};

	// 間合いの並び（kActionWeights の行）
	enum RangeBand : int {
		kClose,
		kMid,
		kFar,
		kBandCount
	};

	// フェーズ × 間合いごとの行動の重み。
	// 届かない攻撃は抽選の前に 0 にするので、残った候補どうしの比で選ばれる
	// （例: フェーズ1の中距離で噛みつきが届かなければ 接近50 : 突進20 になる）
	constexpr int kActionWeights[3][kBandCount][kActionCount] = {
		//  噛みつき 叩きつけ 突進 接近
		{ // フェーズ1: 通常
			{ 60, 30,  0, 10 }, // 近距離: 噛みつき主体
			{ 30,  0, 20, 50 }, // 中距離: 接近か噛みつき
			{  0,  0, 20, 80 }, // 遠距離: 接近優先
		},
		{ // フェーズ2: 激化
			{ 50, 50,  0,  0 }, // 近距離: 噛みつき・叩きつけを均等に
			{ 30,  0, 45, 25 }, // 中距離: 突進が増える
			{  0,  0, 65, 35 },
		},
		{ // フェーズ3: 瀕死・最大攻撃的
			{ 40, 60,  0,  0 },
			{  0, 35, 65,  0 },
			{  0,  0, 85, 15 },
		},
	};

	constexpr const char* kActionStateNames[kActionCount] = {
		BossStateName::Slash,
		BossStateName::HeavySword,
		BossStateName::Rush,
		BossStateName::Approach,
	};
}

BossStateCombatIdle::BossStateCombatIdle(EnemySensorComponent* sensor, EnemyMovementComponent* movement,
	float maxHp, BossBattleMemory* memory)
	: sensor_(sensor), movement_(movement), maxHp_(maxHp), memory_(memory),
	biteTelegraph_(BossStateSlash::GetTelegraph()),
	slamTelegraph_(BossStateHeavySword::GetTelegraph()),
	breathTelegraph_(BossStateBreath::GetTelegraph()) {}

float BossStateCombatIdle::GetCloseRange(Enemy& enemy) {
	// 配置は縦横同じ倍率の前提。攻撃の判定・予兆と同じくオブジェクトのスケールで伸ばす
	return kCloseRange * enemy.GetWorldTransform()->GetWorldScale().x;
}

// 次の行動を選ぶまでの間を置く。
// **ここを 0 にしてはいけない**。攻撃ステートは終わると必ずこのステートへ戻ってくるので、
// 0 にすると戻った次のフレームにまた攻撃を選び、間が一切空かない
void BossStateCombatIdle::Enter(Enemy& enemy) {
	cooldown_ = GetCooldown(GetPhase(enemy.GetHp()));
}
void BossStateCombatIdle::Exit(Enemy&) {}

int BossStateCombatIdle::GetPhase(float hp) const {
	float ratio = hp / maxHp_;
	if (ratio > 0.66f) return 1;
	if (ratio > 0.33f) return 2;
	return 3;
}

// 攻撃と攻撃の間の「溜め」。攻撃モーション自体の長さ（噛みつき0.88秒・叩きつけ1.67秒）に
// これが上乗せされるので、1.2秒ならフェーズ1の攻撃周期はおよそ2秒になる。
// 攻撃頻度を変えたいときはこの3つを触る（大きくするほど攻撃が減る）
float BossStateCombatIdle::GetCooldown(int phase) const {
	if (phase == 1) return 1.2f;
	if (phase == 2) return 0.8f;
	return 0.5f; // フェーズ3: 素早く判断
}

// 必殺技のブレスを撃つかどうか。距離・フェーズとは別枠で判定する。
//   1. HPが半分を切ったら解禁し、その1回目は必ず撃つ（「半分でブレスが来る」を保証）
//   2. 以降は kBreathChance の確率で混ぜる
// どちらも射程（帯の奥の端）に入っているときだけ。外で撃つと炎が届かず空振りするので、
// 解禁していても射程に入るまで待つ（その間は通常の抽選で近づいてくる）。
// 解禁フラグはボス本体が持つ BossBattleMemory に記録するので、
// Idle / Move / CombatIdle のどのインスタンスから来ても同じ記憶を見る
bool BossStateCombatIdle::ShouldUseBreath(const Enemy& enemy, bool inReach) {
	if (!memory_ || maxHp_ <= 0.0f) return false;

	if (!memory_->breathUnlocked && (enemy.GetHp() / maxHp_) <= kBreathHpRatio) {
		memory_->breathUnlocked = true;
		memory_->breathUsed = false;
	}
	if (!memory_->breathUnlocked || !inReach) return false;

	return !memory_->breathUsed || (std::rand() % 100 < kBreathChance);
}

bool BossStateCombatIdle::IsInReach(Enemy& enemy, const AttackTelegraphParams& telegraph, float distance) const {
	// 予兆はスケール1基準で書かれているので、攻撃が実際に出すときと同じ配置スケールを掛ける
	const Vector3 scale = enemy.GetWorldTransform()->GetWorldScale();
	AttackTelegraphParams scaled = telegraph;
	scaled.ApplyScale(scale.x, scale.z);
	// 体は判定が出る直前までプレイヤーの方を向き続けるので、正面方向の距離だけ見ればよい
	return distance <= scaled.GetReach() + kPlayerHalfWidth;
}

void BossStateCombatIdle::Update(Enemy& enemy, float deltaTime) {
	if (!enemy.GetOnGround()) return;

	sensor_->Update(enemy);
	cooldown_ -= deltaTime;
	if (cooldown_ > 0.0f) return;

	const int phase = GetPhase(enemy.GetHp());
	// 予兆は地面に出すので、射程も地面の上の距離で測る（高低差は判定の縦の大きさが受け持つ）
	const float dist = sensor_->GetHorizontalDistanceToPlayer();
	const bool canAttack = enemy.CanAttack();

	// ─── 必殺技: 火炎ブレス ───────────────────────────────────────
	// 距離・フェーズとは別枠。射程に入っていれば間合いを問わず割り込む
	if (canAttack && ShouldUseBreath(enemy, IsInReach(enemy, breathTelegraph_, dist))) {
		memory_->breathUsed = true;
		enemy.ChangeState(BossStateName::Breath);
		return;
	}

	// ─── フェーズ・間合いごとの抽選 ───────────────────────────────
	const float closeRange = GetCloseRange(enemy);
	const float midRange = kMidRange * enemy.GetWorldTransform()->GetWorldScale().x;
	const RangeBand band = (dist < closeRange) ? kClose : (dist < midRange) ? kMid : kFar;

	int weights[kActionCount];
	for (int i = 0; i < kActionCount; ++i) {
		weights[i] = kActionWeights[phase - 1][band][i];
	}

	if (!canAttack) {
		// 攻撃行動が許可されていない敵（トレーニングの攻撃抑制など）は接近だけにする
		for (int i = 0; i < kActionCount; ++i) {
			weights[i] = (i == kApproach) ? 1 : 0;
		}
	} else {
		// 届かない攻撃は候補から外す
		if (!IsInReach(enemy, biteTelegraph_, dist)) weights[kSlash] = 0;
		if (!IsInReach(enemy, slamTelegraph_, dist)) weights[kHeavySword] = 0;
		// 突進は外さない。帯の先まで届かない距離でも「距離を一気に詰める移動」を兼ねていて、
		// 外すと遠くのプレイヤーへは歩いて近づくしかなくなる
	}

	int total = 0;
	for (int weight : weights) {
		total += weight;
	}
	if (total <= 0) {
		// どれも届かず、この間合いに接近の重みも無い。詰めに行く
		enemy.ChangeState(BossStateName::Approach);
		return;
	}

	int roll = std::rand() % total;
	for (int i = 0; i < kActionCount; ++i) {
		if (roll < weights[i]) {
			enemy.ChangeState(kActionStateNames[i]);
			return;
		}
		roll -= weights[i];
	}
}
