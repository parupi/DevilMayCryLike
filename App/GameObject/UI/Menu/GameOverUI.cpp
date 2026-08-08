#include "GameOverUI.h"

#include <Audio/SoundManager.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Graphics/Resource/TextureManager.h>
#include <Input/Input.h>
#include <Utility/DeltaTime.h>

#include <algorithm>

namespace {
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;
	constexpr float kCenterX = kScreenWidth * 0.5f;
}

void GameOverUI::Initialize() {
	TextureManager::GetInstance().LoadTexture("white.png");

	SpriteManager& sprites = SpriteManager::GetInstance();

	backdrop_ = sprites.CreateSprite(SpriteLayer::UI, "gameOverBackdrop", "white.png");
	backdrop_->SetAnchorPoint({ 0.5f, 0.5f });
	backdrop_->SetPosition({ kCenterX, kScreenHeight * 0.5f });
	backdrop_->SetSize({ kScreenWidth, kScreenHeight });

	title_ = sprites.CreateTextLabel(SpriteLayer::UI, "gameOverTitle");
	title_->SetText("YOU DIED");
	title_->SetFontSize(64.0f);
	title_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	title_->SetPosition({ kCenterX, kTitleY });
	title_->SetShadow(true);

	itemList_.Initialize("gameOver", SpriteLayer::UI,
		{ "RETRY", "TO TITLE" },
		{ kCenterX, kItemStartY }, kItemSpacing, kItemFontSize);

	hint_ = sprites.CreateTextLabel(SpriteLayer::UI, "gameOverHint");
	hint_->SetFontSize(22.0f);
	hint_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	hint_->SetPosition({ kCenterX, kHintY });
	hint_->SetShadow(true);

	// 出すまでは何も見せない
	Update();
}

void GameOverUI::Enter() {
	isActive_ = true;
	result_ = Result::None;
	itemList_.SetSelectedIndex(0);
}

void GameOverUI::Update() {
	// ゲームは止まっているので実時間で動かす
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();
	const float step = (kFadeTime > 0.0f) ? (deltaTime / kFadeTime) : 1.0f;

	alpha_ = std::clamp(alpha_ + (isActive_ ? step : -step), 0.0f, 1.0f);

	// 出きるまでは操作を受けない。死んだ勢いの入力で即決定されるのを防ぐ
	if (isActive_ && alpha_ >= 1.0f && result_ == Result::None) {
		navigator_.Update();
		itemList_.UpdateSelection(navigator_);

		if (navigator_.IsDecide()) {
			SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);
			result_ = (static_cast<Item>(itemList_.GetSelectedIndex()) == Item::Retry)
				? Result::Retry : Result::ToTitle;
		}
	} else {
		// 入力の押しっぱなし判定を溜めないよう、受け付けない間も回しておく
		navigator_.Update();
	}

	const bool visible = alpha_ > 0.0f;

	backdrop_->SetColor({ 0.0f, 0.0f, 0.0f, kBackdropAlpha * alpha_ });
	backdrop_->GetRenderState().isVisible = visible;
	backdrop_->Update();

	// 赤みを乗せて、ポーズメニューと取り違えないようにする
	title_->SetColor({ 1.0f, 0.25f, 0.22f, alpha_ });
	title_->GetRenderState().isVisible = visible;
	title_->Update();

	itemList_.Refresh(alpha_, deltaTime);

	const bool usePad = Input::GetInstance().IsConnected();
	hint_->SetText(usePad ? "A : DECIDE" : "SPACE : DECIDE");
	hint_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * 0.8f });
	hint_->GetRenderState().isVisible = visible;
	hint_->Update();
}
