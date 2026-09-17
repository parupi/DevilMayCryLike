#include "FadeTransition.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Utility/DeltaTime.h"

FadeTransition::FadeTransition(const std::string& transitionName)
{
	TextureManager::GetInstance().LoadTexture("white1x1.png");
	name = transitionName;
	sprite_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Persistent, "fadeMask", "white1x1.png");
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
	// フレーム数ではなく経過秒で進める（高リフレッシュレートでも同じ速さになる）
	const float speed = DeltaTime::GetDeltaTime() / kFadeTime;

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
	// スプライトは Persistent レイヤーに登録済みで SpriteManager::DrawUILayers() が
	// 自動描画するため、ここでは何もしない（描くと二重にブレンドされてしまう）。
}
