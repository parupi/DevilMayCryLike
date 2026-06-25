#include "MenuChoices.h"
#include <imgui.h>
#include <cmath>
#include <Utility/DeltaTime.h>
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void MenuChoices::Initialize() {
	TextureManager::GetInstance().LoadTexture("UI/Menu/ToTitle.png");
	TextureManager::GetInstance().LoadTexture("UI/Menu/ToContinue.png");

	toTitle_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "toTitle", "UI/Menu/ToTitle.png");
	toTitle_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	toTitle_->SetPosition({512.0f, 450.0f});

	toContinue_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "toContinue", "UI/Menu/ToContinue.png");
	toContinue_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	toContinue_->SetPosition({512.0f, 300.0f});
}

void MenuChoices::Enter() {
	state_ = ChoicesState::Enter;
}

void MenuChoices::Exit() {
	state_ = ChoicesState::Exit;
}

void MenuChoices::Update() {
	switch (state_) {
	case ChoicesState::Enter:
		alpha_ += DeltaTime::GetDeltaTime() * 1.5f;

		if (alpha_ > 1.0f) {
			alpha_ = 1.0f;
			state_ = ChoicesState::Normal;
		}
		break;
	case ChoicesState::Normal:

		break;
	case ChoicesState::Exit:
		alpha_ -= DeltaTime::GetDeltaTime() * 1.5f;

		if (alpha_ < 0.0f) {
			alpha_ = 0.0f;
			state_ = ChoicesState::Normal;
		}
		break;
	}

	float selectedBrightness = 1.0f;
	float dimBrightness = 0.4f;

	float continueBrightness = (selectedIndex_ == 0) ? selectedBrightness : dimBrightness;
	float titleBrightness = (selectedIndex_ == 1) ? selectedBrightness : dimBrightness;

	// 確認待ち中はタイトル項目を点滅させる (cos で1始まり→0.4~1.0を明滅)
	if (isConfirming_) {
		blinkTimer_ += DeltaTime::GetDeltaTime() * 8.0f;
		titleBrightness = 0.7f + 0.3f * cosf(blinkTimer_);
	}

	toContinue_->SetColor({continueBrightness, continueBrightness, continueBrightness, alpha_});
	toTitle_->SetColor({titleBrightness, titleBrightness, titleBrightness, alpha_});

	toTitle_->Update();
	toContinue_->Update();
}

void MenuChoices::Draw() {
	SpriteManager::GetInstance().DrawSet();
	toTitle_->Draw();
	toContinue_->Draw();
}
