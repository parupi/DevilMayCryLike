#include "ConfirmDialog.h"
#include "MenuNavigator.h"

#include <Audio/SoundManager.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Graphics/Resource/TextureManager.h>
#include <Utility/DeltaTime.h>

#include <algorithm>

namespace {
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;
	constexpr float kCenterX = kScreenWidth * 0.5f;
}

void ConfirmDialog::Initialize(const std::string& idPrefix, SpriteLayer layer) {
	TextureManager::GetInstance().LoadTexture("white.png");

	SpriteManager& sprites = SpriteManager::GetInstance();

	backdrop_ = sprites.CreateSprite(layer, idPrefix + "ConfirmBackdrop", "white.png");
	backdrop_->SetAnchorPoint({ 0.5f, 0.5f });
	backdrop_->SetPosition({ kCenterX, kScreenHeight * 0.5f });
	backdrop_->SetSize({ kScreenWidth, kScreenHeight });

	message_ = sprites.CreateTextLabel(layer, idPrefix + "ConfirmMessage");
	message_->SetFontSize(38.0f);
	message_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	message_->SetPosition({ kCenterX, kMessageY });
	message_->SetShadow(true);

	yes_ = sprites.CreateTextLabel(layer, idPrefix + "ConfirmYes");
	yes_->SetText("YES");
	yes_->SetFontSize(34.0f);
	yes_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	yes_->SetPosition({ kCenterX - kAnswerOffsetX, kAnswerY });
	yes_->SetShadow(true);

	no_ = sprites.CreateTextLabel(layer, idPrefix + "ConfirmNo");
	no_->SetText("NO");
	no_->SetFontSize(34.0f);
	no_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	no_->SetPosition({ kCenterX + kAnswerOffsetX, kAnswerY });
	no_->SetShadow(true);
}

void ConfirmDialog::Open(const std::string& message) {
	message_->SetText(message);
	// 事故を避けるため、必ず NO から始める
	selectedIndex_ = 1;
	isOpen_ = true;
}

void ConfirmDialog::Close() {
	isOpen_ = false;
}

ConfirmDialog::Answer ConfirmDialog::Update(const MenuNavigator& navigator) {
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();
	const float step = (kFadeTime > 0.0f) ? (deltaTime / kFadeTime) : 1.0f;

	alpha_ = std::clamp(alpha_ + (isOpen_ ? step : -step), 0.0f, 1.0f);

	Answer answer = Answer::None;

	// 出きってから操作を受ける。開いた瞬間の入力を拾わないための間でもある
	if (isOpen_ && alpha_ >= 1.0f) {
		if (navigator.IsLeft() || navigator.IsRight()) {
			selectedIndex_ = (selectedIndex_ == 0) ? 1 : 0;
			SoundManager::GetInstance().PlaySE("SwordSlash", 0.25f);
		}

		if (navigator.IsCancel()) {
			SoundManager::GetInstance().PlaySE("SwordSlash", 0.3f);
			answer = Answer::No;
		} else if (navigator.IsDecide()) {
			if (selectedIndex_ == 0) {
				SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);
				answer = Answer::Yes;
			} else {
				SoundManager::GetInstance().PlaySE("SwordSlash", 0.3f);
				answer = Answer::No;
			}
		}
	}

	const bool visible = alpha_ > 0.0f;

	backdrop_->SetColor({ 0.0f, 0.0f, 0.0f, kBackdropAlpha * alpha_ });
	backdrop_->GetRenderState().isVisible = visible;
	backdrop_->Update();

	message_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	message_->GetRenderState().isVisible = visible;
	message_->Update();

	const float yesBrightness = (selectedIndex_ == 0) ? 1.0f : kDimBrightness;
	const float noBrightness = (selectedIndex_ == 1) ? 1.0f : kDimBrightness;

	yes_->SetColor({ yesBrightness, yesBrightness, yesBrightness, alpha_ });
	yes_->GetRenderState().isVisible = visible;
	yes_->Update();

	no_->SetColor({ noBrightness, noBrightness, noBrightness, alpha_ });
	no_->GetRenderState().isVisible = visible;
	no_->Update();

	return answer;
}
