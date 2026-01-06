#include "FadeTransition.h"

FadeTransition::FadeTransition(const std::string& transitionName)
{
	TextureManager::GetInstance()->LoadTexture("white1x1.png");
	name = transitionName;
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("white1x1.png");
	sprite_->SetSize({ 1280.0f, 720.0f });
}

void FadeTransition::Start(bool isFadeOut)
{
	isFadeOut_ = isFadeOut;
	finished_ = false;
	alpha_ = isFadeOut ? 0.0f : 1.0f;
}

void FadeTransition::Update()
{
	const float speed = 0.025f;

	if (isFadeOut_) {
		alpha_ += speed;
		if (alpha_ >= 1.0f) {
			alpha_ = 1.0f;
			finished_ = true;
		}
	} else {
		alpha_ -= speed;
		if (alpha_ <= 0.0f) {
			alpha_ = 0.0f;
			finished_ = true;
		}
	}

	sprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	sprite_->Update();
}

void FadeTransition::Draw()
{
	SpriteManager::GetInstance()->DrawSet();
	sprite_->Draw();
}
