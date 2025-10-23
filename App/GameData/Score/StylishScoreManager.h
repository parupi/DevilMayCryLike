#pragma once
#include <cstdint>
#include <string>

/// <summary>
/// プレイヤーのスタイリッシュスコアを管理するクラス
/// 攻撃やコンボによってスコアを加算し、スコアに応じてランクを決定する
/// </summary>
class StylishScoreManager
{
public:
	StylishScoreManager() = default;
	~StylishScoreManager() = default;

	/// <summary>
	/// 毎フレームの更新処理を行う
	/// スコアの減衰や時間経過によるランク変化などを管理する
	/// </summary>
	void Update();

	/// <summary>
	/// スコアを加算する
	/// </summary>
	/// <param name="scoreNum">加算するスコア値</param>
	/// <param name="attackName">スコアを発生させた攻撃名（任意）</param>
	void AddScore(int32_t scoreNum, std::string attackName = "");

	/// <summary>
	/// プレイヤーがダメージを受けた際の処理
	/// スコアやランクのリセットなどを行う
	/// </summary>
	void OnDamage();

	/// <summary>
	/// 現在のスコアを取得する
	/// </summary>
	/// <returns>現在のスコア値</returns>
	int32_t GetCurrentScore() const { return currentScore_; };

	/// <summary>
	/// 現在のランクを取得する
	/// </summary>
	/// <returns>現在のランク名</returns>
	std::string GetCurrentRank() const { return currentRank_; };

private:
	/// <summary>
	/// 現在のスコアに基づいてランクを更新する
	/// </summary>
	void UpdateRank();

private:
	// 現在のスコア
	int32_t currentScore_ = 0;
	// 現在のランク
	std::string currentRank_;
	// 最後にアクションを行ってからの経過時間
	float timeSinceLastAction_;
};
