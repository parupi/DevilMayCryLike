#pragma once
#include <mutex>
class GameData
{
private:
	static GameData* instance;
	static std::once_flag initInstanceFlag;

	GameData() = default;
	~GameData() = default;
	GameData(const GameData&) = default;
	GameData& operator=(const GameData&) = default;
public:
	// シングルトンインスタンスの取得
	static GameData* GetInstance();

	int32_t GetClearScore() const { return clearScore_; }
	void SetClearScore(int32_t score) { clearScore_ = score; }
	std::string GetClearRank() const { return clearRank_; }
	void SetClearRank(std::string rank) { clearRank_ = rank; }
private:
	int32_t clearScore_ = 1234;
	std::string clearRank_;
};

