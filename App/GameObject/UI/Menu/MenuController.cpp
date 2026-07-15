#include "MenuController.h"
#include <Input/Input.h>
#include <Utility/DeltaTime.h>
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void MenuController::Initialize() {
	TextureManager::GetInstance().LoadTexture("SelectArrow2.png");

	rightArrow_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "rightArrow", "SelectArrow2.png");
	rightArrow_->SetSize({-64.0f, 64.0f});
	rightArrow_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	leftArrow_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "leftArrow", "SelectArrow2.png");
	leftArrow_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	leftArrow_->SetSize({64.0f, 64.0f});

	rightArrow_->SetPosition({820.0f, 300.0f});
	leftArrow_->SetPosition({450.0f, 300.0f});
}

void MenuController::Enter() {
	states_ = MenuStates::Enter;
	alpha_ = 0.0f;
	maskAlpha_ = 0.0f;
	currentY_ = 300.0f;
	arrowTargetY_ = 300.0f;
}

void MenuController::Exit() {
	states_ = MenuStates::Exit;
}

void MenuController::Update() {
	Input* input = &Input::GetInstance();

	// ===== 入力取得 =====
	static bool stickNeutral = true;
	float stickY = input->GetLeftStickY();

	bool stickUp = false;
	bool stickDown = false;

	if (stickNeutral) {
		if (stickY > 0.5f) {
			stickUp = true;
			stickNeutral = false;
		} else if (stickY < -0.5f) {
			stickDown = true;
			stickNeutral = false;
		}
	}

	if (abs(stickY) < 0.3f) {
		stickNeutral = true;
	}

	bool up = input->TriggerKey(DIK_W) || stickUp;
	bool down = input->TriggerKey(DIK_S) || stickDown;
	bool decide = input->TriggerKey(DIK_SPACE) || input->TriggerButton(ButtonA);
	bool cancel = input->TriggerKey(DIK_ESCAPE) || input->TriggerButton(ButtonB);

	// ===== ステート処理 =====
	switch (states_) {
	case MenuStates::Enter:
		alpha_ += DeltaTime::GetDeltaTime();

		if (alpha_ > 1.0f) {
			alpha_ = 1.0f;
			states_ = MenuStates::SelectFirst;
		}
		break;

	case MenuStates::SelectFirst:
		arrowTargetY_ = 300.0f;

		if (up || down) {
			states_ = MenuStates::SelectSecond;
		}

		if (decide) {
			states_ = MenuStates::Exit;
		}
		break;

	case MenuStates::SelectSecond:
		arrowTargetY_ = 450.0f;

		if (up || down) {
			states_ = MenuStates::SelectFirst;
		}

		if (decide) {
			states_ = MenuStates::Confirm;
		}
		break;

	case MenuStates::Confirm:
		arrowTargetY_ = 450.0f;

		if (cancel) {
			states_ = MenuStates::SelectSecond;
		}

		if (decide) {
			states_ = MenuStates::Decision;
		}
		break;

	case MenuStates::Decision:
		maskAlpha_ += DeltaTime::GetDeltaTime() * 3.0f;
		if (maskAlpha_ > 1.0f) {
			maskAlpha_ = 1.0f;
			states_ = MenuStates::SetUp;
		}
		break;

	case MenuStates::Exit:
		maskAlpha_ += DeltaTime::GetDeltaTime();
		if (maskAlpha_ > 0.1f) {
			maskAlpha_ = 0.1f;
		}

		alpha_ -= DeltaTime::GetDeltaTime() * 1.5f;

		if (alpha_ < 0.0f) {
			alpha_ = 0.0f;
			states_ = MenuStates::SetUp;
		}
		break;
	}

	// カーソルY座標を補間
	float t = 12.0f * DeltaTime::GetDeltaTime();
	if (t > 1.0f) t = 1.0f;
	currentY_ += (arrowTargetY_ - currentY_) * t;

	rightArrow_->SetPosition({820.0f, currentY_});
	leftArrow_->SetPosition({450.0f, currentY_});

	rightArrow_->SetColor({1.0f, 1.0f, 1.0f, alpha_});
	leftArrow_->SetColor({1.0f, 1.0f, 1.0f, alpha_});

	rightArrow_->Update();
	leftArrow_->Update();
}
