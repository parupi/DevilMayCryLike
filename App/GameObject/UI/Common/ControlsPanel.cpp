#include "ControlsPanel.h"
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Input/Input.h>
#include <Utility/DeltaTime.h>
#include <algorithm>
#include <string>

namespace {
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;
	constexpr float kCenterX = kScreenWidth * 0.5f;
}

// App/Input と PlayerInput.cpp の実装に合わせた表記
const ControlsPanel::ControlRow ControlsPanel::kRows[] = {
	{ "MOVE",          "LEFT STICK",  "W  A  S  D" },
	{ "CAMERA",        "RIGHT STICK", "ARROW KEYS" },
	{ "JUMP",          "A",           "SPACE" },
	{ "DODGE / DASH",  "RT",          "LEFT SHIFT" },
	{ "ATTACK",        "Y",           "J" },
	{ "STRONG ATTACK", "X",           "K" },
	{ "LOCK ON",       "RB (HOLD)",   "P (HOLD)" },
	{ "MENU",          "START",       "M" },
};
const int32_t ControlsPanel::kRowCount = static_cast<int32_t>(std::size(kRows));

void ControlsPanel::Initialize()
{
	SpriteManager& sprites = SpriteManager::GetInstance();

	backdrop_ = sprites.CreateSprite(SpriteLayer::UI, "controlsBackdrop", "white.png");
	backdrop_->SetAnchorPoint({ 0.5f, 0.5f });
	backdrop_->SetPosition({ kCenterX, kScreenHeight * 0.5f });
	backdrop_->SetSize({ kScreenWidth, kScreenHeight });

	title_ = sprites.CreateTextLabel(SpriteLayer::UI, "controlsTitle");
	title_->SetText("CONTROLS");
	title_->SetFontSize(48.0f);
	title_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	title_->SetPosition({ kCenterX, 130.0f });
	title_->SetShadow(true);

	for (int32_t i = 0; i < kRowCount; ++i) {
		const std::string index = std::to_string(i);
		const float y = kRowStartY + kRowSpacing * i;

		actionLabels_[i] = sprites.CreateTextLabel(SpriteLayer::UI, "controlsAction" + index);
		actionLabels_[i]->SetText(kRows[i].action);
		actionLabels_[i]->SetFontSize(kFontSize);
		actionLabels_[i]->SetAlign(TextAlignX::Left, TextAlignY::Middle);
		actionLabels_[i]->SetPosition({ kActionX, y });
		actionLabels_[i]->SetShadow(true);

		bindLabels_[i] = sprites.CreateTextLabel(SpriteLayer::UI, "controlsBind" + index);
		bindLabels_[i]->SetFontSize(kFontSize);
		bindLabels_[i]->SetAlign(TextAlignX::Right, TextAlignY::Middle);
		bindLabels_[i]->SetPosition({ kBindX, y });
		bindLabels_[i]->SetShadow(true);

		separators_[i] = sprites.CreateSprite(SpriteLayer::UI, "controlsLine" + index, "white.png");
		separators_[i]->SetAnchorPoint({ 0.5f, 0.5f });
		separators_[i]->SetPosition({ kCenterX, y + kRowSpacing * 0.5f - 4.0f });
		separators_[i]->SetSize({ kBindX - kActionX, 1.0f });
	}

	hint_ = sprites.CreateTextLabel(SpriteLayer::UI, "controlsHint");
	hint_->SetFontSize(22.0f);
	hint_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	hint_->SetPosition({ kCenterX, 654.0f });
	hint_->SetShadow(true);

	// 開くまでは何も出さない
	ApplyAlpha();
}

void ControlsPanel::Open()
{
	state_ = State::Appearing;
}

void ControlsPanel::Close()
{
	if (state_ == State::Hidden) return;
	state_ = State::Closing;
}

void ControlsPanel::Update()
{
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();
	const float step = (kFadeTime > 0.0f) ? (deltaTime / kFadeTime) : 1.0f;

	switch (state_) {
	case State::Appearing:
		alpha_ = std::clamp(alpha_ + step, 0.0f, 1.0f);
		if (alpha_ >= 1.0f) state_ = State::Shown;
		break;
	case State::Closing:
		alpha_ = std::clamp(alpha_ - step, 0.0f, 1.0f);
		if (alpha_ <= 0.0f) state_ = State::Hidden;
		break;
	default:
		break;
	}

	ApplyAlpha();
}

void ControlsPanel::ApplyAlpha()
{
	// パッドを挿し直されてもすぐ追従できるよう、表記の出し分けは毎フレーム見る
	const bool usePad = Input::GetInstance().IsConnected();
	const bool visible = (state_ != State::Hidden);

	backdrop_->SetColor({ 0.0f, 0.0f, 0.0f, kBackdropAlpha * alpha_ });
	backdrop_->GetRenderState().isVisible = visible;
	backdrop_->Update();

	title_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	title_->GetRenderState().isVisible = visible;
	title_->Update();

	for (int32_t i = 0; i < kRowCount; ++i) {
		actionLabels_[i]->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
		actionLabels_[i]->GetRenderState().isVisible = visible;
		actionLabels_[i]->Update();

		// 割り当ての側は少し落として、操作名との主従を付ける
		bindLabels_[i]->SetText(usePad ? kRows[i].pad : kRows[i].keyboard);
		bindLabels_[i]->SetColor({ 0.75f, 0.82f, 0.92f, alpha_ });
		bindLabels_[i]->GetRenderState().isVisible = visible;
		bindLabels_[i]->Update();

		separators_[i]->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * 0.18f });
		separators_[i]->GetRenderState().isVisible = visible;
		separators_[i]->Update();
	}

	hint_->SetText(usePad ? "B : BACK" : "ESC : BACK");
	hint_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * 0.8f });
	hint_->GetRenderState().isVisible = visible;
	hint_->Update();
}
