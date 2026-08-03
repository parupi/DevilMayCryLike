#include "GameUI.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void GameUI::Initialize()
{
	TextureManager::GetInstance().LoadTexture("UI/attack.png");
	TextureManager::GetInstance().LoadTexture("UI/Jump.png");
	TextureManager::GetInstance().LoadTexture("UI/LockOn.png");

	attackUI_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "attack", "UI/attack.png");
	jumpUI_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "jump", "UI/Jump.png");
	lockOnUI_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "lockOn", "UI/LockOn.png");

	attackUI_->SetSize({256.0f, 64.0f});
	jumpUI_->SetSize({ 256.0f, 64.0f });
	lockOnUI_->SetSize({ 256.0f, 64.0f });

	attackUI_->SetPosition({ 30.0f, 560.0f });
	jumpUI_->SetPosition({ 24.0f, 630.0f });
	lockOnUI_->SetPosition({ 32.0f, 480.0f });
}

void GameUI::Update()
{
	attackUI_->Update();
	jumpUI_->Update();
	lockOnUI_->Update();
}

