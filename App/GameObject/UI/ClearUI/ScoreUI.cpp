#include "ScoreUI.h"
#include <cstdint>
#include <GameData/GameData.h>
#include <string>
#include <base/utility/DeltaTime.h>

void ScoreUI::Initialize()
{
	targetScore_ = GameData::GetInstance()->GetClearScore();
	int32_t digits = std::to_string(std::abs(targetScore_)).size();

	for (int32_t i = 0; i < digits; i++) {
		std::unique_ptr<Sprite> num = std::make_unique<Sprite>();
		num->Initialize("Numbers.png");
		num->SetSize({ 64.0f, 64.0f });
		num->SetUVSize({ 0.1f, 1.0f });
		scoreNums_.push_back(std::move(num));
	}

	countSpeed_ = targetScore_* 0.5;
}

void ScoreUI::Update()
{
	if (!isStart_) return;

	// スコアを徐々に近づける（LERP 風）
	if (currentScore_ < targetScore_) {
		currentScore_ += static_cast<int>(countSpeed_ * DeltaTime::GetDeltaTime());
		if (currentScore_ > targetScore_) {
			currentScore_ = targetScore_;
		}
	} else if (currentScore_ > targetScore_) {
		currentScore_ -= static_cast<int>(countSpeed_ * DeltaTime::GetDeltaTime());
		if (currentScore_ < targetScore_) {
			currentScore_ = targetScore_;
		}
	} else {
		isFinish_ = true;
	}

	// ↓ currentScore_ を用いて数字を描画
	DrawScore(currentScore_);
}

void ScoreUI::DrawScore(int32_t score)
{
	std::string score_str = std::to_string(score);
	int32_t digits = score_str.size();

	for (int32_t i = 0; i < digits; i++) {
		int digit = score_str.at(i) - '0';
		float u = digit * 0.1f;

		scoreNums_[i]->SetUVPosition({ u, 0.0f });
		scoreNums_[i]->SetPosition({ 220.0f + i * 36.0f, 310.0f });
		scoreNums_[i]->Update();
	}
}

void ScoreUI::Draw()
{
	for (auto& num : scoreNums_) {
		num->Draw();
	}
}

void ScoreUI::Start()
{
	isStart_ = true;
}
