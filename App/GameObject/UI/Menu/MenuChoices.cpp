#include "MenuChoices.h"
#include <imgui.h>
#include <base/utility/DeltaTime.h>

void MenuChoices::Initialize()
{
	TextureManager::GetInstance()->LoadTexture("UI/Menu/ToTitle.png");
	TextureManager::GetInstance()->LoadTexture("UI/Menu/ToContinue.png");

	toTitle_ = std::make_unique<Sprite>();
	toTitle_->Initialize("UI/Menu/ToTitle.png");
	toTitle_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	toTitle_->SetPosition({ 512.0f, 450.0f });

	toContinue_ = std::make_unique<Sprite>();
	toContinue_->Initialize("UI/Menu/ToContinue.png");
	toContinue_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	toContinue_->SetPosition({ 512.0f, 300.0f });
}

void MenuChoices::Enter()
{
	//toTitle_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	//toContinue_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	state_ = ChoicesState::Enter;
}

void MenuChoices::Exit()
{
	//toTitle_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	//toContinue_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	state_ = ChoicesState::Exit;
}

void MenuChoices::Update()
{
	//Vector2 titlePos = toTitle_->GetPosition();
	//Vector2 conPos = toContinue_->GetPosition();

	//ImGui::Begin("Sprite");
	//ImGui::DragFloat2("translate1", &titlePos.x);
	//ImGui::DragFloat2("translate2", &conPos.x);
	//ImGui::End();

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

	toTitle_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	toContinue_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });

	toTitle_->Update();
	toContinue_->Update();
}

void MenuChoices::Draw()
{
	SpriteManager::GetInstance()->DrawSet();
	toTitle_->Draw();
	toContinue_->Draw();
}
