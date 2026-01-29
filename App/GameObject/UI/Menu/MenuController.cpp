#include "MenuController.h"
#include <imgui.h>
#include <input/Input.h>
#include <base/utility/DeltaTime.h>

void MenuController::Initialize()
{
	TextureManager::GetInstance()->LoadTexture("SelectArrow2.png");

	rightArrow_ = std::make_unique<Sprite>();
	rightArrow_->Initialize("SelectArrow2.png");
	rightArrow_->SetSize({ -64.0f, 64.0f });
	rightArrow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	leftArrow_ = std::make_unique<Sprite>();
	leftArrow_->Initialize("SelectArrow2.png");
	leftArrow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	leftArrow_->SetSize({ 64.0f, 64.0f });

	rightArrow_->SetPosition({ 820.0f, 300.0f });
	leftArrow_->SetPosition({ 450.0f, 300.0f });

	musk_ = std::make_unique<Sprite>();
	musk_->Initialize("white.png");
	musk_->SetSize({330.0f, 64.0f});

}

void MenuController::Enter()
{
	states_ = MenuStates::Enter;
}

void MenuController::Exit()
{
	states_ = MenuStates::Exit;
}

void MenuController::Update()
{
	switch (states_) {
	case MenuStates::Enter:
		alpha_ += DeltaTime::GetDeltaTime();

		if (alpha_ > 1.0f) {
			alpha_ = 1.0f;
			states_ = MenuStates::SelectFirst;
		}
	
		break;
	case MenuStates::SelectFirst:
		rightArrow_->SetPosition({820.0f, 300.0f});
		leftArrow_->SetPosition({ 450.0f, 300.0f });
		musk_->SetPosition({ 470.0f, 300.0f });

		if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_S)/* || Input::GetInstance()->PushButton(ButtonStart)*/) {
			states_ = MenuStates::SelectSecond;
		}

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			states_ = MenuStates::Exit;
		}
		break;
	case MenuStates::SelectSecond:
		rightArrow_->SetPosition({ 820.0f, 450.0f });
		leftArrow_->SetPosition({ 450.0f, 450.0f });
		musk_->SetPosition({ 470.0f, 450.0f });

		if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_S)) {
			states_ = MenuStates::SelectFirst;
		}

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			states_ = MenuStates::Decision;
		}
		break;
	case MenuStates::Decision:
		muskAlpha_ += DeltaTime::GetDeltaTime() * 3.0f;
		if (muskAlpha_ > 1.0f) {
			muskAlpha_ = 1.0f;
			states_ = MenuStates::SetUp;
		}
		
		break;
	case MenuStates::Exit:
		muskAlpha_ += DeltaTime::GetDeltaTime();
		if (muskAlpha_ > 0.1f) {
			muskAlpha_ = 0.1f;
		}

		alpha_ -= DeltaTime::GetDeltaTime() * 1.5f;

		if (alpha_ < 0.0f) {
			alpha_ = 0.0f;
			states_ = MenuStates::SetUp;
		}

		break;
	}

	rightArrow_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	leftArrow_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	musk_->SetColor({ 1.0f, 1.0f, 1.0f, muskAlpha_ });

	rightArrow_->Update();
	leftArrow_->Update();
	musk_->Update();
}

void MenuController::Draw()
{
	if (states_ == MenuStates::Decision || states_ == MenuStates::Exit) {
		//musk_->Draw();
	}

	rightArrow_->Draw();
	leftArrow_->Draw();
}
