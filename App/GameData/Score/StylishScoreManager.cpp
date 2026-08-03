#include "StylishScoreManager.h"
#include <Utility/DeltaTime.h>
#include <Debugger/GlobalVariables.h>
#include <GameData/GameData.h>
#include <algorithm>
#ifdef _DEBUG
#endif

void StylishScoreManager::Initialize()
{
	RegisterParams();
}

void StylishScoreManager::RegisterParams()
{
	global_ = &GlobalVariables::GetInstance();
	// 保存済みの設定があれば読み込む
	global_->LoadFile("Score", "StylishScore");

	// 基礎点
	global_->AddItem("StylishScore", "PointHit", pointHit_);
	global_->AddItem("StylishScore", "PointStrong", pointStrong_);
	global_->AddItem("StylishScore", "PointAirCombo", pointAirCombo_);
	global_->AddItem("StylishScore", "PointJustDodge", pointJustDodge_);
	global_->AddItem("StylishScore", "PointParry", pointParry_);
	global_->AddItem("StylishScore", "PointKill", pointKill_);
	// コンボ
	global_->AddItem("StylishScore", "ComboWindow", comboWindow_);
	global_->AddItem("StylishScore", "ComboMulMax", comboMulMax_);
	global_->AddItem("StylishScore", "ComboMulHitsForMax", comboMulHitsForMax_);
	// 同一攻撃ペナルティ
	global_->AddItem("StylishScore", "RepeatPenaltyStep", repeatPenaltyStep_);
	global_->AddItem("StylishScore", "RepeatPenaltyMin", repeatPenaltyMin_);
	// 多様性ボーナス
	global_->AddItem("StylishScore", "DiversityBonusPer", diversityBonusPer_);
	global_->AddItem("StylishScore", "DiversityMaxStacks", diversityMaxStacks_);
	// 被弾ペナルティ
	global_->AddItem("StylishScore", "DamagePenaltyScale", damagePenaltyScale_);
	// 自動戦闘
	global_->AddItem("StylishScore", "AutoBattleEnabled", autoBattleEnabled_);
	global_->AddItem("StylishScore", "BattleRange", battleRange_);
	global_->AddItem("StylishScore", "AutoBattleEndTime", autoBattleEndTime_);
	global_->AddItem("StylishScore", "AutoBattleMinPeak", autoBattleMinPeak_);
	// 時間減衰
	global_->AddItem("StylishScore", "DecayIdleTime", decayIdleTime_);
	global_->AddItem("StylishScore", "DecaySpeed", decaySpeed_);
	global_->AddItem("StylishScore", "BoundaryHoldTime", boundaryHoldTime_);
	// ランク境界
	global_->AddItem("StylishScore", "RankC", rankC_);
	global_->AddItem("StylishScore", "RankB", rankB_);
	global_->AddItem("StylishScore", "RankA", rankA_);
	global_->AddItem("StylishScore", "RankS", rankS_);
	global_->AddItem("StylishScore", "RankSS", rankSS_);
	global_->AddItem("StylishScore", "RankSSS", rankSSS_);

	ApplyParams();
}

void StylishScoreManager::ApplyParams()
{
	if (!global_) return;

	pointHit_ = global_->GetValueRef<float>("StylishScore", "PointHit");
	pointStrong_ = global_->GetValueRef<float>("StylishScore", "PointStrong");
	pointAirCombo_ = global_->GetValueRef<float>("StylishScore", "PointAirCombo");
	pointJustDodge_ = global_->GetValueRef<float>("StylishScore", "PointJustDodge");
	pointParry_ = global_->GetValueRef<float>("StylishScore", "PointParry");
	pointKill_ = global_->GetValueRef<float>("StylishScore", "PointKill");
	comboWindow_ = global_->GetValueRef<float>("StylishScore", "ComboWindow");
	comboMulMax_ = global_->GetValueRef<float>("StylishScore", "ComboMulMax");
	comboMulHitsForMax_ = global_->GetValueRef<int32_t>("StylishScore", "ComboMulHitsForMax");
	repeatPenaltyStep_ = global_->GetValueRef<float>("StylishScore", "RepeatPenaltyStep");
	repeatPenaltyMin_ = global_->GetValueRef<float>("StylishScore", "RepeatPenaltyMin");
	diversityBonusPer_ = global_->GetValueRef<float>("StylishScore", "DiversityBonusPer");
	diversityMaxStacks_ = global_->GetValueRef<int32_t>("StylishScore", "DiversityMaxStacks");
	damagePenaltyScale_ = global_->GetValueRef<float>("StylishScore", "DamagePenaltyScale");
	autoBattleEnabled_ = global_->GetValueRef<bool>("StylishScore", "AutoBattleEnabled");
	battleRange_ = global_->GetValueRef<float>("StylishScore", "BattleRange");
	autoBattleEndTime_ = global_->GetValueRef<float>("StylishScore", "AutoBattleEndTime");
	autoBattleMinPeak_ = global_->GetValueRef<float>("StylishScore", "AutoBattleMinPeak");
	decayIdleTime_ = global_->GetValueRef<float>("StylishScore", "DecayIdleTime");
	decaySpeed_ = global_->GetValueRef<float>("StylishScore", "DecaySpeed");
	boundaryHoldTime_ = global_->GetValueRef<float>("StylishScore", "BoundaryHoldTime");
	rankC_ = global_->GetValueRef<float>("StylishScore", "RankC");
	rankB_ = global_->GetValueRef<float>("StylishScore", "RankB");
	rankA_ = global_->GetValueRef<float>("StylishScore", "RankA");
	rankS_ = global_->GetValueRef<float>("StylishScore", "RankS");
	rankSS_ = global_->GetValueRef<float>("StylishScore", "RankSS");
	rankSSS_ = global_->GetValueRef<float>("StylishScore", "RankSSS");
}

void StylishScoreManager::Update()
{
	// エディタでの変更を毎フレーム反映する
	ApplyParams();

	const float dt = DeltaTime::GetDeltaTime();
	timeSinceLastAction_ += dt;

	// コンボ終了判定：一定時間ヒットが途切れたらコンボをリセット
	if (comboCount_ > 0) {
		comboTimer_ += dt;
		if (comboTimer_ > comboWindow_) {
			EndCombo();
		}
	}

	// 時間減衰：何もしない時間が続くとポイントを徐々に減らす（ランク境界で一定時間ホールド）
	UpdateDecay(dt);

	// 暗黙戦闘（強制戦闘以外）の終了判定：敵から一定距離・時間離れたら終了する。
	// 敵を倒した場合も「近くに生存敵がいない」状態になるのでここで終了する。
	if (battleActive_ && !battleForced_) {
		bool enemyNearby = (nearestEnemyDist_ <= battleRange_);
		if (enemyNearby) {
			disengageTimer_ = 0.0f;
		} else {
			disengageTimer_ += dt;
			if (disengageTimer_ >= autoBattleEndTime_) {
				EndBattle();
			}
		}
	}

	// 戦闘中はこの戦闘のピーク（＝その戦闘のスコア）を更新する
	if (battleActive_) {
		battlePeak_ = (std::max)(battlePeak_, stylePoint_);
	}

	// クリア画面などで参照する共有データへは「最終スコア（各戦闘の平均）」を反映する
	int32_t finalScore = GetFinalScore();
	GameData::GetInstance().SetClearScore(finalScore);
	GameData::GetInstance().SetClearRank(RankToCode(ComputeRank(static_cast<float>(finalScore))));

// 表示は App/Editor/Windows/StylishWindow.cpp が担当する
}

void StylishScoreManager::OnAttackHit(const AttackHitContext& ctx)
{
	// 攻撃を当てたので（戦闘中でなければ）暗黙戦闘を開始する
	NotifyAttackLanded();

	// 攻撃したのでコンボと減衰タイマーをリフレッシュ
	timeSinceLastAction_ = 0.0f;
	comboTimer_ = 0.0f;
	++comboCount_;

	// 同一攻撃ペナルティ：直前と同じ攻撃なら連続回数を増やす
	if (!ctx.attackName.empty() && ctx.attackName == lastAttackName_) {
		++repeatCount_;
	} else {
		repeatCount_ = 0;
		lastAttackName_ = ctx.attackName;
	}

	// 多様性ボーナス：現コンボで使った攻撃の種類を記録
	if (!ctx.attackName.empty()) {
		comboAttackSet_.insert(ctx.attackName);
	}

	// 基礎点：状況に応じて切り替える（空中 > 強攻撃 > 通常）
	float base = pointHit_;
	if (ctx.isAir) {
		base = pointAirCombo_;
	} else if (ctx.isStrong) {
		base = pointStrong_;
	}

	// 加点 = 基礎点 × コンボ倍率 × 同一攻撃ペナルティ × 多様性ボーナス × 敵補正
	float gain = base
		* CalcComboMultiplier()
		* CalcRepeatPenalty()
		* CalcDiversityBonus()
		* ctx.enemyMultiplier;

	stylePoint_ += gain;
	UpdateRank();
}

void StylishScoreManager::OnEnemyKilled(float enemyMultiplier)
{
	timeSinceLastAction_ = 0.0f;
	comboTimer_ = 0.0f;

	// 撃破はコンボ倍率と敵補正のみ乗せる（連発・多様性の対象外）
	float gain = pointKill_ * CalcComboMultiplier() * enemyMultiplier;
	stylePoint_ += gain;
	UpdateRank();
}

void StylishScoreManager::OnJustDodge()
{
	timeSinceLastAction_ = 0.0f;
	comboTimer_ = 0.0f;
	comboAttackSet_.insert("JustDodge");

	stylePoint_ += pointJustDodge_ * CalcComboMultiplier();
	UpdateRank();
}

void StylishScoreManager::OnParry()
{
	timeSinceLastAction_ = 0.0f;
	comboTimer_ = 0.0f;
	comboAttackSet_.insert("Parry");

	stylePoint_ += pointParry_ * CalcComboMultiplier();
	UpdateRank();
}

void StylishScoreManager::OnDamage()
{
	// 被弾：スタイルポイントを大きく減らし、コンボを打ち切る
	stylePoint_ *= damagePenaltyScale_;
	// 被弾は即時の減点なのでランク境界ホールドは解除する
	holdingAtBoundary_ = false;
	boundaryHoldTimer_ = 0.0f;
	EndCombo();
	UpdateRank();
}

void StylishScoreManager::EndCombo()
{
	comboCount_ = 0;
	comboTimer_ = 0.0f;
	repeatCount_ = 0;
	lastAttackName_.clear();
	comboAttackSet_.clear();
}

void StylishScoreManager::BeginBattle()
{
	// 強制戦闘イベントによる戦闘開始
	StartBattle(true);
}

void StylishScoreManager::StartBattle(bool forced)
{
	// 進行中の戦闘があれば先に確定させる（暗黙戦闘→強制戦闘への切り替えなど）
	if (battleActive_) {
		EndBattle();
	}

	// この戦闘のスタイルポイントをゼロから計測し直す
	battleActive_ = true;
	battleForced_ = forced;
	battlePeak_ = 0.0f;
	stylePoint_ = 0.0f;
	timeSinceLastAction_ = 0.0f;
	disengageTimer_ = 0.0f;
	holdingAtBoundary_ = false;
	boundaryHoldTimer_ = 0.0f;
	EndCombo();
	UpdateRank();
}

void StylishScoreManager::NotifyAttackLanded()
{
	// 攻撃を当てたら、まだ戦闘中でなければ暗黙戦闘を開始する
	if (!battleActive_ && autoBattleEnabled_) {
		StartBattle(false);
	}
}

void StylishScoreManager::EndBattle()
{
	if (!battleActive_) return;

	// この戦闘で到達したピークをその戦闘のスコアとして記録する。
	// 強制戦闘は常に記録。暗黙戦闘は軽微な小競り合い（ピークが低い）を平均から除外する。
	if (battleForced_ || battlePeak_ >= autoBattleMinPeak_) {
		battleScores_.push_back(battlePeak_);
	}
	battleActive_ = false;
	battleForced_ = false;
	disengageTimer_ = 0.0f;
}

int32_t StylishScoreManager::GetFinalScore() const
{
	// これまでの各戦闘スコアの平均。戦闘中はその戦闘の暫定ピークも1件として含める
	float sum = 0.0f;
	int32_t count = 0;
	for (float s : battleScores_) {
		sum += s;
		++count;
	}
	if (battleActive_) {
		sum += battlePeak_;
		++count;
	}
	if (count == 0) return 0;
	return static_cast<int32_t>(sum / static_cast<float>(count));
}

float StylishScoreManager::CalcComboMultiplier() const
{
	// コンボ数が多いほど1.0→comboMulMaxまで上昇（1ヒット目は1.0）
	if (comboMulHitsForMax_ <= 1 || comboCount_ == 0) {
		return 1.0f;
	}
	float t = static_cast<float>(comboCount_ - 1) / static_cast<float>(comboMulHitsForMax_ - 1);
	t = std::clamp(t, 0.0f, 1.0f);
	return 1.0f + t * (comboMulMax_ - 1.0f);
}

float StylishScoreManager::CalcRepeatPenalty() const
{
	// 同じ攻撃を連発するほど倍率が下がる（下限あり）
	float factor = 1.0f - static_cast<float>(repeatCount_) * repeatPenaltyStep_;
	return (std::max)(repeatPenaltyMin_, factor);
}

float StylishScoreManager::CalcDiversityBonus() const
{
	// 現コンボで使った攻撃の種類数に応じてボーナス（1種類目は加算なし）
	int32_t distinct = static_cast<int32_t>(comboAttackSet_.size());
	int32_t stacks = std::clamp(distinct - 1, 0, diversityMaxStacks_);
	return 1.0f + static_cast<float>(stacks) * diversityBonusPer_;
}

StyleRank StylishScoreManager::ComputeRank(float points) const
{
	if (points >= rankSSS_)     return StyleRank::SSS;
	else if (points >= rankSS_) return StyleRank::SS;
	else if (points >= rankS_)  return StyleRank::S;
	else if (points >= rankA_)  return StyleRank::A;
	else if (points >= rankB_)  return StyleRank::B;
	else if (points >= rankC_)  return StyleRank::C;
	return StyleRank::D;
}

float StylishScoreManager::LowerThresholdOf(StyleRank rank) const
{
	switch (rank) {
	case StyleRank::SSS: return rankSSS_;
	case StyleRank::SS:  return rankSS_;
	case StyleRank::S:   return rankS_;
	case StyleRank::A:   return rankA_;
	case StyleRank::B:   return rankB_;
	case StyleRank::C:   return rankC_;
	default:             return 0.0f; // D（これ以上下は無い）
	}
}

void StylishScoreManager::UpdateDecay(float dt)
{
	// 十分な無操作時間が経つまでは減衰しない
	if (timeSinceLastAction_ <= decayIdleTime_ || stylePoint_ <= 0.0f) {
		holdingAtBoundary_ = false;
		boundaryHoldTimer_ = 0.0f;
		return;
	}

	// 現在ランクを維持できる下限スコア（＝1つ下のランクとの境界）
	const float lower = LowerThresholdOf(currentRank_);
	const bool canHold = (currentRank_ != StyleRank::D); // Dより下は無いのでホールドしない

	if (canHold && holdingAtBoundary_) {
		// 境界でスコアを止めて猶予を与える（この間はランクが下がらない）
		stylePoint_ = lower;
		boundaryHoldTimer_ += dt;
		if (boundaryHoldTimer_ >= boundaryHoldTime_) {
			// 猶予終了：境界を割ってランクダウンさせる
			holdingAtBoundary_ = false;
			boundaryHoldTimer_ = 0.0f;
			stylePoint_ = (std::max)(0.0f, lower - decaySpeed_ * dt);
		}
	} else {
		const float proposed = stylePoint_ - decaySpeed_ * dt;
		if (canHold && stylePoint_ > lower && proposed <= lower) {
			// このフレームでランク境界に到達 → 境界で止めてホールド開始
			stylePoint_ = lower;
			holdingAtBoundary_ = true;
			boundaryHoldTimer_ = 0.0f;
		} else {
			// 通常の減衰
			stylePoint_ = (std::max)(0.0f, proposed);
		}
	}

	UpdateRank();
}

void StylishScoreManager::UpdateRank()
{
	StyleRank newRank = ComputeRank(stylePoint_);

	if (newRank != currentRank_) {
		StyleRank old = currentRank_;
		currentRank_ = newRank;
		OnRankChanged(old, newRank);
	}
}

void StylishScoreManager::OnRankChanged(StyleRank oldRank, StyleRank newRank)
{
	// ランクアップ/ダウンの演出フック。
	// SE・ボイス・UI演出はここから鳴らす（Phase B で実装予定）。
	(void)oldRank;
	(void)newRank;
}

std::string StylishScoreManager::RankToCode(StyleRank rank)
{
	switch (rank) {
	case StyleRank::SSS: return "SSS";
	case StyleRank::SS:  return "SS";
	case StyleRank::S:   return "S";
	case StyleRank::A:   return "A";
	case StyleRank::B:   return "B";
	case StyleRank::C:   return "C";
	default:             return "D";
	}
}

const char* StylishScoreManager::RankToDisplayName(StyleRank rank)
{
	switch (rank) {
	case StyleRank::SSS: return "Smokin' Sexy Style!!";
	case StyleRank::SS:  return "Smokin' Style";
	case StyleRank::S:   return "Stylish";
	case StyleRank::A:   return "Atomic";
	case StyleRank::B:   return "Brutal";
	case StyleRank::C:   return "Crazy";
	default:             return "Dull";
	}
}
