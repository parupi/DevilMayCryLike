#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

class GlobalVariables;

/// <summary>
/// スタイリッシュランク（Style Rank）。値が大きいほど高評価。
/// </summary>
enum class StyleRank {
	D, C, B, A, S, SS, SSS
};

/// <summary>
/// 攻撃ヒット時にスコアへ渡す状況情報。
/// 状況ごとに評価を変える（空中・強攻撃・敵の強さなど）ために使う。
/// </summary>
struct AttackHitContext {
	std::string attackName;       // 攻撃名（同一攻撃ペナルティ・多様性ボーナスの判定に使う）
	bool isAir = false;           // 空中攻撃か
	bool isStrong = false;        // 強攻撃（打ち上げ・吹き飛ばし）か
	float enemyMultiplier = 1.0f; // 敵ごとの補正（雑魚1.0 / ボス2.0 など）
};

/// <summary>
/// Devil May Cry風のスタイリッシュランクを管理するクラス。
/// 「どれだけ格好良く戦ったか」を評価し、リアルタイムにランクを上下させる。
///
/// 加点 = 基礎点 × コンボ倍率 × 同一攻撃ペナルティ × 多様性ボーナス × 敵補正。
/// 何もしないと時間減衰し、被弾でスタイルポイントが大きく減る。
/// </summary>
class StylishScoreManager
{
public:
	StylishScoreManager() = default;
	~StylishScoreManager() = default;

	/// <summary>パラメータの読み込み・登録を行う（生成直後に一度呼ぶ）。</summary>
	void Initialize();

	/// <summary>毎フレームの更新（時間減衰・コンボ終了判定・UI・GameData同期）。</summary>
	void Update();

	// ======================
	// 戦闘セッション（戦闘ごとにスコアを決め、最終スコアはその平均）
	// ======================

	/// <summary>強制戦闘の開始。スタイルポイントとコンボをリセットして計測を始める。</summary>
	void BeginBattle();
	/// <summary>戦闘終了。この戦闘のスコア（ピーク値）を記録し、最終スコアの平均へ反映する。</summary>
	void EndBattle();
	/// <summary>戦闘中かどうか（HUDの表示切り替えに使う）。</summary>
	bool IsBattleActive() const { return battleActive_; }
	/// <summary>これまでの戦闘スコアの平均（戦闘中はその戦闘の暫定ピークも含める）。</summary>
	int32_t GetFinalScore() const;
	/// <summary>
	/// 最寄りの生存敵との水平距離を供給する（GameSceneから毎フレーム）。
	/// 暗黙戦闘（強制戦闘以外）を「敵から一定距離離れたら終了」と判定するのに使う。
	/// </summary>
	void SetNearestEnemyDistance(float dist) { nearestEnemyDist_ = dist; }

	// ======================
	// スコア加算・減算のトリガー
	// ======================

	/// <summary>攻撃がヒットしたときに呼ぶ（メインの加点）。</summary>
	void OnAttackHit(const AttackHitContext& ctx);
	/// <summary>敵を撃破したときに呼ぶ。</summary>
	void OnEnemyKilled(float enemyMultiplier = 1.0f);
	/// <summary>ジャスト回避に成功したときに呼ぶ（将来のドッジシステム用）。</summary>
	void OnJustDodge();
	/// <summary>パリィに成功したときに呼ぶ（将来のパリィシステム用）。</summary>
	void OnParry();
	/// <summary>被弾したときに呼ぶ（スタイルポイント減少＋コンボリセット）。</summary>
	void OnDamage();

	// ======================
	// 参照用ゲッター
	// ======================

	int32_t GetCurrentScore() const { return static_cast<int32_t>(stylePoint_); }
	// ランクの短縮コード（"D"〜"SSS"）。UIやGameDataとの互換のためコードで返す
	std::string GetCurrentRank() const { return RankToCode(currentRank_); }
	StyleRank GetRank() const { return currentRank_; }
	// ランクの表示名（"Dull" 〜 "Smokin' Sexy Style!!"）
	const char* GetRankDisplayName() const { return RankToDisplayName(currentRank_); }
	uint32_t GetComboCount() const { return comboCount_; }
	float GetComboMultiplier() const { return CalcComboMultiplier(); }

#ifdef _DEBUG
	/// <summary>
	/// エディタ表示用の状態スナップショット。
	/// 調整パラメータは GlobalVariables 側にあるので App/Editor/Windows/StylishWindow.cpp が
	/// 直接いじる。デバッグ表示のためだけに内部状態を1つずつ公開したくないので、まとめて渡す。
	/// </summary>
	struct EditorStatus {
		bool battleActive;
		bool battleForced;
		float battlePeak;
		size_t battleCount;
		float nearestEnemyDist;
		float battleRange;
		float disengageTimer;
		bool holdingAtBoundary;
		float boundaryHoldTimer;
		uint32_t comboCount;
		float comboMultiplier;
		std::string lastAttackName;
		uint32_t repeatCount;
		float repeatPenalty;
		size_t varietyCount;
		float diversityBonus;
	};
	EditorStatus MakeEditorStatus() const {
		return { battleActive_, battleForced_, battlePeak_, battleScores_.size(),
			nearestEnemyDist_, battleRange_, disengageTimer_,
			holdingAtBoundary_, boundaryHoldTimer_,
			comboCount_, CalcComboMultiplier(),
			lastAttackName_, repeatCount_, CalcRepeatPenalty(),
			comboAttackSet_.size(), CalcDiversityBonus() };
	}
#endif // _DEBUG

private:
	// パラメータの登録・反映（GlobalVariablesエディタ連携）
	void RegisterParams();
	void ApplyParams();

	// 現在のスタイルポイントからランクを更新し、変化があれば演出フックを呼ぶ
	void UpdateRank();
	void OnRankChanged(StyleRank oldRank, StyleRank newRank);
	// スタイルポイントからランクを求める（境界判定の共通処理）
	StyleRank ComputeRank(float points) const;
	// 指定ランクを維持できる下限スコア（時間減衰のランク境界ホールドに使う）
	float LowerThresholdOf(StyleRank rank) const;

	// 戦闘を開始する（forced=true は強制戦闘、false は攻撃ヒットによる暗黙戦闘）
	void StartBattle(bool forced);
	// 攻撃を当てたときに呼ぶ：暗黙戦闘を開始する
	void NotifyAttackLanded();
	// 時間減衰の処理（ランク境界での一定時間ホールドを含む）
	void UpdateDecay(float dt);

	// コンボを終了させる（コンボ数・同一攻撃・多様性をリセット）
	void EndCombo();

	// 各種倍率の計算
	float CalcComboMultiplier() const;   // コンボ数が多いほど上昇
	float CalcRepeatPenalty() const;     // 同じ攻撃の連発で低下
	float CalcDiversityBonus() const;    // 多彩な攻撃で上昇

	static std::string RankToCode(StyleRank rank);
	static const char* RankToDisplayName(StyleRank rank);

private:
	GlobalVariables* global_ = nullptr;

	// ===== 実行時の状態 =====
	float stylePoint_ = 0.0f;                    // スタイルポイント（内部は実数で滑らかに減衰）
	StyleRank currentRank_ = StyleRank::D;        // 現在のランク

	float timeSinceLastAction_ = 0.0f;            // 最後の加点からの経過時間（時間減衰用）

	uint32_t comboCount_ = 0;                     // 現在のコンボ数
	float comboTimer_ = 0.0f;                     // 最後のヒットからの経過時間（コンボ終了用）

	std::string lastAttackName_;                  // 直前にヒットした攻撃名（同一攻撃ペナルティ用）
	uint32_t repeatCount_ = 0;                    // 同一攻撃の連続回数

	std::unordered_set<std::string> comboAttackSet_; // 現コンボ中に使った攻撃名（多様性ボーナス用）

	// ===== 戦闘セッション =====
	bool battleActive_ = false;              // 戦闘中か
	bool battleForced_ = false;              // 現在の戦闘が強制戦闘イベントによるものか
	float battlePeak_ = 0.0f;                // 現在の戦闘で到達した最大スタイルポイント
	std::vector<float> battleScores_;        // 終了した各戦闘のスコア（ピーク値）
	float disengageTimer_ = 0.0f;            // 敵から離れている継続時間（暗黙戦闘の終了判定）
	float nearestEnemyDist_ = 1e9f;          // 最寄りの生存敵との水平距離（GameSceneから供給）

	// 時間減衰のランク境界ホールド
	bool holdingAtBoundary_ = false;         // ランク境界でスコアを止めている最中か
	float boundaryHoldTimer_ = 0.0f;         // 境界で止めている継続時間

	// ===== 調整パラメータ（GlobalVariablesエディタから編集） =====
	// 基礎点
	float pointHit_ = 20.0f;        // 攻撃ヒット
	float pointStrong_ = 40.0f;     // 強攻撃
	float pointAirCombo_ = 60.0f;   // 空中コンボ
	float pointJustDodge_ = 80.0f;  // ジャスト回避
	float pointParry_ = 100.0f;     // パリィ
	float pointKill_ = 30.0f;       // 敵撃破

	// コンボ
	float comboWindow_ = 2.0f;          // この秒数ヒットが途切れるとコンボ終了
	float comboMulMax_ = 2.0f;          // コンボ倍率の最大値
	int32_t comboMulHitsForMax_ = 30;   // 倍率が最大に達するヒット数

	// 同一攻撃ペナルティ
	float repeatPenaltyStep_ = 0.25f;   // 連発1回ごとの低下量
	float repeatPenaltyMin_ = 0.1f;     // ペナルティ下限倍率

	// 多様性ボーナス
	float diversityBonusPer_ = 0.15f;   // 攻撃種類1つごとの加算倍率
	int32_t diversityMaxStacks_ = 4;    // 多様性ボーナスの最大段数

	// 被弾ペナルティ
	float damagePenaltyScale_ = 0.5f;   // 被弾時にスタイルポイントへ掛ける倍率

	// 自動戦闘（強制戦闘イベント以外での戦闘検知）
	bool autoBattleEnabled_ = true;     // 攻撃ヒットで暗黙の戦闘を開始するか
	float battleRange_ = 20.0f;         // この距離内に生存敵がいれば交戦中とみなす
	float autoBattleEndTime_ = 3.0f;    // 敵から離れてこの秒数経過で暗黙戦闘を終了
	float autoBattleMinPeak_ = 50.0f;   // 平均に含める最低ピーク（軽微な小競り合いを除外）

	// 時間減衰
	float decayIdleTime_ = 3.0f;        // 無操作がこの秒数続くと減衰開始
	float decaySpeed_ = 100.0f;         // 減衰速度（ポイント/秒）
	float boundaryHoldTime_ = 1.5f;     // 減衰時にランク境界でスコアを止める秒数

	// ランク境界
	float rankC_ = 100.0f;
	float rankB_ = 300.0f;
	float rankA_ = 700.0f;
	float rankS_ = 1200.0f;
	float rankSS_ = 1800.0f;
	float rankSSS_ = 2500.0f;
};
