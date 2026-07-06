#include "Tutorial.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void Tutorial::Initialize(const std::string& name, uint32_t maxCounter) {
	// チュートリアルの初期化処理
	state_ = State::Inactive;
	// 進行度の最大値を設定
	maxCounter_ = maxCounter;
	// スプライトの生成と初期設定
	tutorialImage = SpriteManager::GetInstance().CreateAnimatedSprite(SpriteLayer::UI, "TutorialImage", "Tutorial/" + name + ".gif");
	tutorialImage->GetSprite()->SetAnchorPoint({0.5f, 0.5f});
	tutorialImage->GetSprite()->SetPosition({210.0f, 340.0f});
	tutorialImage->GetSprite()->SetSize({350.0f, 240.0f});
	tutorialImage->GetSprite()->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	tutorialImage->GetSprite()->GetRenderState().blendMode = BlendMode::kNormal;
	// スプライトの生成と初期設定
	tutorialText = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "TutorialText", "Tutorial/" + name + ".png");
	tutorialText->SetAnchorPoint({0.5f, 0.5f});
	tutorialText->SetPosition({200.0f, 520.0f});
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
		float alpha = tutorialImage->GetSprite()->GetColor().w;
		// アルファ値を徐々に増加させる
		alpha += 0.01f;

		if (alpha >= 1.0f) {
			alpha = 1.0f; // 最大値を超えないようにする
			state_ = State::Active; // アクティブな状態に遷移
		}

		// スプライトのカラーにアルファ値を適用
		tutorialImage->GetSprite()->SetColor({1.0f, 1.0f, 1.0f, alpha});
		tutorialText->SetColor({1.0f, 1.0f, 1.0f, alpha});
	}
	break;
	case State::Active:
		// アクティブな状態の処理
		break;
	case State::End:
		// 終了中の処理
	{
		float alpha = tutorialImage->GetSprite()->GetColor().w;
		// アルファ値を徐々に減少させる
		alpha -= 0.01f;

		if (alpha <= 0.0f) {
			alpha = 0.0f; // 最小値を下回らないようにする
			state_ = State::Inactive; // 非アクティブな状態に遷移
		}

		// スプライトのカラーにアルファ値を適用
		tutorialImage->GetSprite()->SetColor({1.0f, 1.0f, 1.0f, alpha});
		tutorialText->SetColor({1.0f, 1.0f, 1.0f, alpha});
	}
	break;
	}

	Vector2 pos = tutorialText->GetPosition();
	Vector2 size = tutorialText->GetSize();
	ImGui::Begin("TutorialSprite");
	ImGui::DragFloat2("Pos", &pos.x, 0.1f);
	ImGui::DragFloat2("Size", &size.x, 0.1f);
	ImGui::End();
	tutorialText->SetPosition(pos);
	tutorialText->SetSize(size);

	tutorialImage->Update();
	tutorialText->Update();
}

void Tutorial::Start() {
	state_ = State::Start;
	// 進行度を戻しておく
	counter_ = 0;
}

void Tutorial::End() {
	state_ = State::End;
}

bool Tutorial::StepTutorial() {
	// 進行度を加算
	counter_++;
	// 最大以上ならTrue
	if (counter_ >= maxCounter_) {
		return true;
	}
	return false;
}
