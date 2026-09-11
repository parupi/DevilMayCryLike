#pragma once
#include <memory>
#include "RankUI.h"
#include "ScoreUI.h"
#include <Graphics/Rendering/Sprite/Sprite.h>

class TextLabel;

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

	// 描画は SpriteManager が UI レイヤーとして自動で行うため Draw() は持たない

	float EaseOutBack(float t);

private:
	State state_ = State::ResultDrop;

	// 各UIのアニメ進行度
	float timer_ = 0.0f;

	// もともとの位置・スケールを保持
	Vector2 resultDefaultPos_;
	Vector2 stageDefaultPos_;
	Vector2 scoreDefaultPos_;

	// 見出しの文字（以前は文字入りの PNG だった）
	TextLabel* resultUI_ = nullptr;
	TextLabel* stageNumUI_ = nullptr;
	TextLabel* score_ = nullptr;


	std::unique_ptr<ScoreUI> scoreUI_;

	std::unique_ptr<RankUI> rankUI_;
};

