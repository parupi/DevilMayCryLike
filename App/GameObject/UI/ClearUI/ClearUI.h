#pragma once
#include <memory>
#include "RankUI.h"
#include "ScoreUI.h"
#include <2d/Sprite.h>

enum class State {
	ResultDrop,
	StageDrop,
	ScoreDrop,
	Finished
};

class ClearUI
{
public:
	ClearUI() = default;
	~ClearUI() = default;

	void Initialize();

	void Update();

	void Draw();

	float EaseOutBack(float t);

private:
	State state_ = State::ResultDrop;

	// 各UIのアニメ進行度
	float timer_ = 0.0f;

	// もともとの位置・スケールを保持
	Vector2 resultDefaultPos_;
	Vector2 stageDefaultPos_;
	Vector2 scoreDefaultPos_;

	std::unique_ptr<Sprite> resultUI_;
	std::unique_ptr<Sprite> stageNumUI_;
	std::unique_ptr<Sprite> score_;


	std::unique_ptr<ScoreUI> scoreUI_;

	std::unique_ptr<RankUI> rankUI_;
};

