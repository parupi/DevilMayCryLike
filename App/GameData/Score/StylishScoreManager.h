#pragma once
#include <cstdint>
#include <string>
class StylishScoreManager
{
public:
	StylishScoreManager() = default;
	~StylishScoreManager() = default;
	// 毎フレーム更新処理
	void Update();
	// スコア加算
	void AddScore(int32_t scoreNum, std::string attackName = "");
	// ダメージを受けた時
	void OnDamage();

	int32_t GetCurrentScore() const { return currentScore_; };
	std::string GetCurrentRank() const { return currentRank_; };

private:
	// スコアからランクを決定する処理
	void UpdateRank();

private:
	int32_t currentScore_ = 0;
	std::string currentRank_;

	float timeSinceLastAction_;
};

