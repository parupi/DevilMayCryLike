#include "GameUI.h"
//#include <imgui.h>

void GameUI::Initialize()
{
	attackUI_ = std::make_unique<Sprite>();
	jumpUI_ = std::make_unique<Sprite>();
	lockOnUI_ = std::make_unique<Sprite>();

	TextureManager::GetInstance()->LoadTexture("UI/attack.png");
	TextureManager::GetInstance()->LoadTexture("UI/Jump.png");
	TextureManager::GetInstance()->LoadTexture("UI/LockOn.png");

	attackUI_->Initialize("UI/attack.png");
	jumpUI_->Initialize("UI/Jump.png");
	lockOnUI_->Initialize("UI/LockOn.png");

	attackUI_->SetSize({256.0f, 64.0f});
	jumpUI_->SetSize({ 256.0f, 64.0f });
	lockOnUI_->SetSize({ 256.0f, 64.0f });

	attackUI_->SetPosition({ 30.0f, 560.0f });
	jumpUI_->SetPosition({ 24.0f, 630.0f });
	lockOnUI_->SetPosition({ 32.0f, 480.0f });
}

void GameUI::Update()
{
	//Vector2 spritePos = attackUI_->GetPosition();
	//Vector2 spriteSize = attackUI_->GetSize();
	//ImGui::Begin("Sprite");;
	//ImGui::DragFloat2("pos", &spritePos.x);
	//ImGui::DragFloat2("size", &spriteSize.x);
	//ImGui::End();
	//attackUI_->SetPosition(spritePos);
	//attackUI_->SetSize(spriteSize);

	attackUI_->Update();
	jumpUI_->Update();
	lockOnUI_->Update();
}

void GameUI::Draw()
{
	attackUI_->Draw();
	jumpUI_->Draw();
	lockOnUI_->Draw();
}
