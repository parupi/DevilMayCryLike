#include "TutorialDecoration.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void TutorialDecoration::Initialize() {
	musk_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "TutorialMusk", "white.png");
	musk_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});
	musk_->SetSize({426.0f, 720.0f});
}

void TutorialDecoration::Update() {
	switch (state_) {
	case State::Start:
	{
		float alpha = musk_->GetColor().w;
		// アルファ値を徐々に減少させる
		alpha += 0.01f;

		if (alpha >= 0.7f) {
			alpha = 0.7f; // 最大値を上回らないようにする
			state_ = State::Active; // アクティブな状態に遷移
		}

		// スプライトのカラーにアルファ値を適用
		musk_->SetColor({0.0f, 0.0f, 0.0f, alpha});
		musk_->SetColor({0.0f, 0.0f, 0.0f, alpha});
	}
	break;
	case State::End:
	{
		float alpha = musk_->GetColor().w;
		// アルファ値を徐々に減少させる
		alpha -= 0.01f;

		if (alpha <= 0.0f) {
			alpha = 0.0f; // 最小値を上回らないようにする
			state_ = State::Inactive; // 非アクティブな状態に遷移
		}

		// スプライトのカラーにアルファ値を適用
		musk_->SetColor({0.0f, 0.0f, 0.0f, alpha});
		musk_->SetColor({0.0f, 0.0f, 0.0f, alpha});
	}
	break;
	default:

		break;
	}

	musk_->Update();
}

void TutorialDecoration::Start() {
	state_ = State::Start;
}

void TutorialDecoration::End() {
	state_ = State::End;
}
