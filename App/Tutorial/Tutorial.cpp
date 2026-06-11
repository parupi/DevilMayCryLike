#include "Tutorial.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void Tutorial::Initialize() {
	// チュートリアルの初期化処理
	state_ = State::Inactive;
	// スプライトの生成と初期設定
	tutorialImage = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "TutorialImage", "white.png");
	tutorialImage->SetAnchorPoint({0.5f, 0.5f});
	tutorialImage->SetPosition({400.0f, 300.0f});
	tutorialImage->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	// スプライトの生成と初期設定
	tutorialText = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "TutorialText", "white.png");
	tutorialText->SetAnchorPoint({0.5f, 0.5f});
	tutorialText->SetPosition({400.0f, 400.0f});
	tutorialText->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
}

void Tutorial::Update() {
	switch (state_) {
	case State::Inactive:
		// 非アクティブな状態の処理
		break;
	case State::Start:
		// 開始中の処理
	{
		float alpha = tutorialImage->GetColor().w;
		// アルファ値を徐々に増加させる
		alpha += 0.01f;

		if (alpha >= 1.0f) {
			alpha = 1.0f; // 最大値を超えないようにする
			state_ = State::Active; // アクティブな状態に遷移
		}

		// スプライトのカラーにアルファ値を適用
		tutorialImage->SetColor({1.0f, 1.0f, 1.0f, alpha});
		tutorialText->SetColor({1.0f, 1.0f, 1.0f, alpha});
	}
	break;
	case State::Active:
		// アクティブな状態の処理
		break;
	case State::End:
		// 終了中の処理
	{
		float alpha = tutorialImage->GetColor().w;
		// アルファ値を徐々に減少させる
		alpha -= 0.01f;

		if (alpha <= 0.0f) {
			alpha = 0.0f; // 最小値を下回らないようにする
			state_ = State::Inactive; // 非アクティブな状態に遷移
		}

		// スプライトのカラーにアルファ値を適用
		tutorialImage->SetColor({1.0f, 1.0f, 1.0f, alpha});
		tutorialText->SetColor({1.0f, 1.0f, 1.0f, alpha});
	}
	break;
	}

	tutorialImage->Update();
	tutorialText->Update();
}

void Tutorial::Start() {
	state_ = State::Start;
}

void Tutorial::End() {
	state_ = State::End;
}
