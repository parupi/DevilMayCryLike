#include "BossStateCombatIdle.h"
#include "BossStateSlash.h"
#include "BossStateHeavySword.h"
#include "BossStateBreath.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Component/EnemyMovementComponent.h"
#include <cmath>
#include <numbers>

namespace {
	// 抽選する行動の並び（kActionWeights の列）
	enum BossAction : int {
		kSlash,      // 噛みつき
		kHeavySword, // 叩きつけ
		kRush,       // 突進
		kApproach,   // 接近
		kActionCount,          // ここまでが抽選表の列
		kBreath = kActionCount // 必殺技は抽選表の外で選ぶ
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

	constexpr const char* kActionStateNames[kActionCount + 1] = {
		BossStateName::Slash,
		BossStateName::HeavySword,
		BossStateName::Rush,
		BossStateName::Approach,
		BossStateName::Breath,
	};

	// 配置スケール。配置は縦横同じ倍率の前提で、攻撃の判定・予兆と同じくオブジェクトのスケールを使う
	float GetRangeScale(Enemy& enemy) {
		return enemy.GetWorldTransform()->GetWorldScale().x;
	}
}

BossStateCombatIdle::BossStateCombatIdle(EnemySensorComponent* sensor, EnemyMovementComponent* movement,
	float maxHp, BossBattleMemory* memory)
	: sensor_(sensor), movement_(movement), maxHp_(maxHp), memory_(memory),
	biteTelegraph_(BossStateSlash::GetTelegraph()),
	slamTelegraph_(BossStateHeavySword::GetTelegraph()),
	breathTelegraph_(BossStateBreath::GetTelegraph()) {}

float BossStateCombatIdle::GetCloseRange(Enemy& enemy) {
	return kCloseRange * GetRangeScale(enemy);
}

void BossStateCombatIdle::Enter(Enemy& enemy) {
	sensor_->Update(enemy);
	const int phase = GetPhase(enemy.GetHp());
	footwork_ = Footwork::Hold;
	chainPending_ = false;

	if (phase >= 3 && memory_->lastAction == kRush) {
		// 突進で抜けた直後。Update が振り返りを待って噛みつきを繋げる
		chainPending_ = true;
		cooldown_ = kChainWindow;
		return;
	}

	if (memory_->lastAction == kApproach) {
		// 歩き終わりに立ち止まらない。届いていればすぐ攻撃、届いていなければまた詰める
		cooldown_ = kAfterApproachWait;
		return;
	}

	// 次の行動を選ぶまでの間を置く。
	// **ここを 0 にしてはいけない**。攻撃ステートは終わると必ずこのステートへ戻ってくるので、
	// 0 にすると戻った次のフレームにまた攻撃を選び、間が一切空かない
	cooldown_ = GetCooldown(phase);
	PickFootwork(enemy);
}

void BossStateCombatIdle::Exit(Enemy& enemy) {
	// 足さばきの速度を次のステートへ持ち越さない
	movement_->Stop(enemy);
}

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

int BossStateCombatIdle::RollPercent() {
	return std::uniform_int_distribution<int>(0, 99)(memory_->rng);
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

	return !memory_->breathUsed || (RollPercent() < kBreathChance);
}

bool BossStateCombatIdle::IsInReach(Enemy& enemy, const AttackTelegraphParams& telegraph, float distance) const {
	// 予兆はスケール1基準で書かれているので、攻撃が実際に出すときと同じ配置スケールを掛ける
	const Vector3 scale = enemy.GetWorldTransform()->GetWorldScale();
	AttackTelegraphParams scaled = telegraph;
	scaled.ApplyScale(scale.x, scale.z);
	// 体は判定が出る直前までプレイヤーの方へ向き直り続けるので、正面方向の距離だけ見ればよい
	return distance <= scaled.GetReach() + kPlayerHalfWidth;
}

bool BossStateCombatIdle::IsFacingPlayer(Enemy& enemy) const {
	Vector3 toPlayer = sensor_->GetDirectionToPlayer();
	toPlayer.y = 0.0f;
	if (Length(toPlayer) < 0.001f) return true;
	toPlayer = Normalize(toPlayer);

	const float cosLimit = std::cos(kChainFacingDegrees * std::numbers::pi_v<float> / 180.0f);
	return Dot(enemy.GetForward(), toPlayer) >= cosLimit;
}

void BossStateCombatIdle::PickFootwork(Enemy& enemy) {
	footwork_ = Footwork::Hold;
	if (!enemy.GetPlayer()) return;

	const float dist = sensor_->GetHorizontalDistanceToPlayer();
	const float closeRange = GetCloseRange(enemy);
	const float midRange = kMidRange * GetRangeScale(enemy);
	const int roll = RollPercent();

	if (dist < closeRange * kCrowdedRatio) {
		// 懐に入られている。下がって間合いを作るか、回り込む
		footwork_ = (roll < 60) ? Footwork::Retreat : Footwork::Strafe;
	} else if (dist < midRange) {
		// にらみ合い。横へ回り込みながら次を狙う
		footwork_ = (roll < 55) ? Footwork::Strafe : Footwork::Hold;
	}
	// 遠距離はその場で構える（次の判断で詰めに来る）

	strafeDir_ = (RollPercent() < 50) ? 1.0f : -1.0f;
}

void BossStateCombatIdle::UpdateFootwork(Enemy& enemy) {
	// 体が大きいほど一歩も大きいので、配置スケールで速さを伸ばす
	const float scale = GetRangeScale(enemy);
	switch (footwork_) {
	case Footwork::Strafe:
		movement_->MoveSideways(enemy, kStrafeSpeed * scale, strafeDir_);
		break;
	case Footwork::Retreat:
		movement_->MoveAway(enemy, kRetreatSpeed * scale);
		break;
	default:
		movement_->Stop(enemy);
		break;
	}
}

void BossStateCombatIdle::StartAction(Enemy& enemy, int action) {
	memory_->lastAction = action;
	enemy.ChangeState(kActionStateNames[action]);
}

void BossStateCombatIdle::Update(Enemy& enemy, float deltaTime) {
	if (!enemy.GetOnGround()) return;

	sensor_->Update(enemy);
	const int phase = GetPhase(enemy.GetHp());

	// ─── フェーズ移行: 咆哮 ───────────────────────────────────────
	// トレーニングでHPを戻したときは記録も戻す（また下がったときに吼え直す）
	if (phase < memory_->shownPhase) {
		memory_->shownPhase = phase;
	}
	// 新しいフェーズへ入ったら、待ち時間を待たずに吼える
	if (phase > memory_->shownPhase) {
		memory_->shownPhase = phase;
		memory_->lastAction = BossBattleMemory::kNoAction;
		enemy.ChangeState(BossStateName::Roar);
		return;
	}

	cooldown_ -= deltaTime;

	if (chainPending_) {
		// ─── 連携: 突進 → 噛みつき（フェーズ3）─────────────────────
		// 抜けた先で止まって振り返る。正面にとらえて届けば、待ち時間を待たずに噛みつく
		movement_->Stop(enemy);
		if (enemy.CanAttack() && IsFacingPlayer(enemy)
			&& IsInReach(enemy, biteTelegraph_, sensor_->GetHorizontalDistanceToPlayer())) {
			chainPending_ = false;
			StartAction(enemy, kSlash);
			return;
		}
		if (cooldown_ > 0.0f) return;
		// 振り返りきれない・届かないまま時間切れ。いつもの判断へ
		chainPending_ = false;
	} else {
		UpdateFootwork(enemy);
		if (cooldown_ > 0.0f) return;
	}

	ChooseAction(enemy, phase);
}

void BossStateCombatIdle::ChooseAction(Enemy& enemy, int phase) {
	// 予兆は地面に出すので、射程も地面の上の距離で測る（高低差は判定の縦の大きさが受け持つ）
	const float dist = sensor_->GetHorizontalDistanceToPlayer();
	const bool canAttack = enemy.CanAttack();

	// ─── 必殺技: 火炎ブレス ───────────────────────────────────────
	// 距離・フェーズとは別枠。射程に入っていれば間合いを問わず割り込む
	if (canAttack && ShouldUseBreath(enemy, IsInReach(enemy, breathTelegraph_, dist))) {
		memory_->breathUsed = true;
		StartAction(enemy, kBreath);
		return;
	}

	// ─── フェーズ・間合いごとの抽選 ───────────────────────────────
	const float rangeScale = GetRangeScale(enemy);
	const RangeBand band = (dist < kCloseRange * rangeScale) ? kClose
		: (dist < kMidRange * rangeScale) ? kMid : kFar;

	float weights[kActionCount];
	for (int i = 0; i < kActionCount; ++i) {
		weights[i] = static_cast<float>(kActionWeights[phase - 1][band][i]);
	}

	if (!canAttack) {
		// 攻撃行動が許可されていない敵（トレーニングの攻撃抑制など）は接近だけにする
		for (int i = 0; i < kActionCount; ++i) {
			weights[i] = (i == kApproach) ? 1.0f : 0.0f;
		}
	} else {
		// 届かない攻撃は候補から外す
		if (!IsInReach(enemy, biteTelegraph_, dist)) weights[kSlash] = 0.0f;
		if (!IsInReach(enemy, slamTelegraph_, dist)) weights[kHeavySword] = 0.0f;
		// 突進は外さない。帯の先まで届かない距離でも「距離を一気に詰める移動」を兼ねていて、
		// 外すと遠くのプレイヤーへは歩いて近づくしかなくなる

		// 直前と同じ攻撃は選ばれにくくする（接近は続けてよい）
		const int last = memory_->lastAction;
		if (last >= 0 && last < kApproach) {
			weights[last] *= kRepeatWeightScale;
		}
	}

	float total = 0.0f;
	for (float weight : weights) {
		total += weight;
	}
	if (total <= 0.0f) {
		// どれも届かず、この間合いに接近の重みも無い。詰めに行く
		StartAction(enemy, kApproach);
		return;
	}

	float roll = std::uniform_real_distribution<float>(0.0f, total)(memory_->rng);
	for (int i = 0; i < kActionCount; ++i) {
		if (weights[i] <= 0.0f) continue;
		if (roll < weights[i]) {
			StartAction(enemy, i);
			return;
		}
		roll -= weights[i];
	}
	// 浮動小数の端数で抜けたときは、重みの残っている最後の候補
	for (int i = kActionCount - 1; i >= 0; --i) {
		if (weights[i] > 0.0f) {
			StartAction(enemy, i);
			return;
		}
	}
}
