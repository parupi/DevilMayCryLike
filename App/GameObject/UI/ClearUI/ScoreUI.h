#pragma once
#include <vector>
#include <2d/Sprite.h>
#include <memory>
class ScoreUI
{
public:
	ScoreUI() = default;
	~ScoreUI() = default;

	void Initialize();

	void Update();
	// 描画用に数字を整える
	void DrawScore(int32_t score);

	void Draw();

	void Start();

	bool isFinished() const { return isFinish_; }
private:

	std::vector<std::unique_ptr<Sprite>> scoreNums_;
	
	bool isStart_ = false;
	bool isFinish_ = false;
	int32_t currentScore_ = 0;  // 画面に表示しているスコア
	int32_t targetScore_ = 0;   // 実際のスコア（GameData から取得）
	float countSpeed_ = 300.0f;  // 1秒でどれくらい進めるか（調整可）
};

