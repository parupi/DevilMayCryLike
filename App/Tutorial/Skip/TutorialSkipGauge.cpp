#include "TutorialSkipGauge.h"

#include "Graphics/Rendering/Sprite/Sprite.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Text/TextLabel.h"
#include "Input/Input.h"

#include <algorithm>

namespace {
	// 中身の色。チュートリアルのパネルの文字に合わせた温かい白
	constexpr float kFillR = 0.95f;
	constexpr float kFillG = 0.86f;
	constexpr float kFillB = 0.62f;
	constexpr float kTrackAlpha = 0.65f;
	constexpr float kFrameAlpha = 0.45f;
}

void TutorialSkipGauge::Initialize()
{
	SpriteManager& sprites = SpriteManager::GetInstance();

	// 奥から順に作る（同じレイヤーでは後から作ったものが手前に出る）
	const float left = kBarRight - kBarWidth;

	frame_ = sprites.CreateSprite(SpriteLayer::UI, "tutorialSkipFrame", "white.png");
	frame_->SetAnchorPoint({ 0.0f, 0.5f });
	frame_->SetPosition({ left - kFrameThickness, kBarY });
	frame_->SetSize({ kBarWidth + kFrameThickness * 2.0f, kBarHeight + kFrameThickness * 2.0f });

	track_ = sprites.CreateSprite(SpriteLayer::UI, "tutorialSkipTrack", "white.png");
	track_->SetAnchorPoint({ 0.0f, 0.5f });
	track_->SetPosition({ left, kBarY });
	track_->SetSize({ kBarWidth, kBarHeight });

	fill_ = sprites.CreateSprite(SpriteLayer::UI, "tutorialSkipFill", "white.png");
	fill_->SetAnchorPoint({ 0.0f, 0.5f });
	fill_->SetPosition({ left, kBarY });
	fill_->SetSize({ 0.0f, kBarHeight });

	label_ = sprites.CreateTextLabel(SpriteLayer::UI, "tutorialSkipLabel");
	label_->SetFontSize(kFontSize);
	label_->SetAlign(TextAlignX::Right, TextAlignY::Middle);
	label_->SetPosition({ kBarRight, kBarY - kBarHeight * 0.5f - kLabelGap });
	label_->SetShadow(true);

	Apply();
}

void TutorialSkipGauge::Flash()
{
	flash_ = 1.0f;
}

void TutorialSkipGauge::Update(float progress, bool show, bool resident, bool allowed, float deltaTime)
{
	progress_ = std::clamp(progress, 0.0f, 1.0f);
	allowed_ = allowed;

	if (!allowed_) {
		// ポーズの暗幕の下に半端に残らないよう、操作中でなければ即座に消す
		alpha_ = 0.0f;
	} else {
		// 押している間ははっきり、チュートリアル中は控えめに出しっぱなしにする。
		// 離したあとゲージが残っている間は、空になるまで今の濃さのまま待つ
		float target = resident ? kIdleAlpha : 0.0f;
		if (show) {
			target = 1.0f;
		} else if (progress_ > 0.0f) {
			target = (std::max)(target, alpha_);
		}
		if (alpha_ < target) {
			alpha_ = (std::min)(target, alpha_ + kFadeInSpeed * deltaTime);
		} else {
			alpha_ = (std::max)(target, alpha_ - kFadeOutSpeed * deltaTime);
		}
	}
	flash_ = std::clamp(flash_ - kFlashDecay * deltaTime, 0.0f, 1.0f);

	// どのボタンを押せばよいかは、パッドが繋がっているかで出し分ける
	const std::string text = Input::GetInstance().IsConnected()
		? "START 長押し：スキップ"
		: "M 長押し：スキップ";
	if (text != labelText_) {
		labelText_ = text;
		label_->SetText(labelText_);
	}

	Apply();
}

void TutorialSkipGauge::Apply()
{
	const bool visible = allowed_ && alpha_ > 0.0f;

	// 光っている間は中身を白へ寄せる
	const float r = kFillR + (1.0f - kFillR) * flash_;
	const float g = kFillG + (1.0f - kFillG) * flash_;
	const float b = kFillB + (1.0f - kFillB) * flash_;

	frame_->SetColor({ 1.0f, 1.0f, 1.0f, kFrameAlpha * alpha_ * (1.0f + flash_) });
	track_->SetColor({ 0.0f, 0.0f, 0.0f, kTrackAlpha * alpha_ });
	fill_->SetColor({ r, g, b, alpha_ });
	fill_->SetSize({ kBarWidth * progress_, kBarHeight });
	label_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });

	frame_->GetRenderState().isVisible = visible;
	track_->GetRenderState().isVisible = visible;
	// 幅 0 の矩形は描かない
	fill_->GetRenderState().isVisible = visible && progress_ > 0.0f;
	label_->GetRenderState().isVisible = visible;

	frame_->Update();
	track_->Update();
	fill_->Update();
	label_->Update();
}
