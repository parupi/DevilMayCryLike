#include "TutorialDecoration.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void TutorialDecoration::Initialize() {
	musk_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "TutorialMusk", "white.png");
	musk_->SetColor({0.0f, 0.0f, 0.0f, 0.5f});
	musk_->SetSize({426.0f, 720.0f});
}

void TutorialDecoration::Update() {
	musk_->Update();
}
