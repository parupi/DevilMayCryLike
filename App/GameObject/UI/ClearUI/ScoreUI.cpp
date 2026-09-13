#include "ScoreUI.h"
#include <cstdint>
#include <GameData/GameData.h>
#include <string>
#include <Utility/DeltaTime.h>
#include "Graphics/Rendering/Sprite/SpriteManager.h"

namespace {
	// 見出し（ClearUI）と同じ「黒い文字に白い影」
	const Vector4 kNumberColor = { 0.05f, 0.05f, 0.05f, 1.0f };
	const Vector4 kNumberShadowColor = { 1.0f, 1.0f, 1.0f, 0.9f };
	const Vector2 kNumberShadowOffset = { 3.0f, 3.0f };
}

void ScoreUI::Initialize()
{
	targetScore_ = GameData::GetInstance().GetClearScore();

	// 数えている途中の値をそのまま文字にする。数え始めるまでは空なので何も出ない
	scoreLabel_ = SpriteManager::GetInstance().CreateTextLabel(SpriteLayer::UI, "clearScore");
	scoreLabel_->SetFontSize(60.0f);
	scoreLabel_->SetAlign(TextAlignX::Left, TextAlignY::Middle);
	// 「Score :」（ClearUI が x=222 まで右揃えで置いている）の右隣
	scoreLabel_->SetPosition({ 236.0f, 342.0f });
	scoreLabel_->SetColor(kNumberColor);
	scoreLabel_->SetShadow(true, kNumberShadowOffset, kNumberShadowColor);

	countSpeed_ = targetScore_ * 0.5f;
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
	scoreLabel_->SetText(std::to_string(score));
	scoreLabel_->Update();
}

void ScoreUI::Start()
{
	isStart_ = true;
}
