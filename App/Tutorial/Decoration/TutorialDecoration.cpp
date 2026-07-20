#include "TutorialDecoration.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void TutorialDecoration::Initialize() {
	mask_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "TutorialMask", "white.png");
	mask_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});
	mask_->SetSize({426.0f, 720.0f});

	upDivider_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "upperDivider", "UI/Menu/UpperDivider.png");
	upDivider_->SetPosition({200.0f, 640.0f});
	upDivider_->SetSize({360.0f, -190.0f});
	upDivider_->SetAnchorPoint({0.5f, 0.5f});
	upDivider_->SetDissolveThreshold(1.0f);
	//upDivider_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});

	underDivider_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "underDivider", "UI/Menu/UnderDivider.png");
	underDivider_->SetPosition({200.0f, 200.0f});
	underDivider_->SetSize({360.0f, 190.0f});
	underDivider_->SetAnchorPoint({0.5f, 0.5f});
	underDivider_->SetDissolveThreshold(1.0f);
	//underDivider_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
}

void TutorialDecoration::Update() {
	switch (state_) {
	case State::Start:
	{
		float alpha = mask_->GetColor().w;
		// ディゾルブの強さを取得
		float threshold = upDivider_->GetDissolveThreshold();
		// アルファ値を徐々に増加させる
		alpha += 0.01f;
		// ディゾルブは徐々によわくする
		threshold -= 0.01f;

		if (threshold <= 0.0f) {
			threshold = 0.0f; // 最大値を上回らないようにする
			state_ = State::Active; // アクティブな状態に遷移
		}

		if (alpha >= 0.7f) {
			alpha = 0.7f;
		}

		// スプライトのカラーにアルファ値を適用
		mask_->SetColor({0.0f, 0.0f, 0.0f, alpha});
		upDivider_->SetDissolveThreshold(threshold);
		underDivider_->SetDissolveThreshold(threshold);
	}
	break;
	case State::End:
	{
		float alpha = mask_->GetColor().w;
		// ディゾルブの強さを取得
		float threshold = upDivider_->GetDissolveThreshold();
		// アルファ値を徐々に減少させる
		alpha -= 0.01f;
		// ディゾルブは徐々につよくする
		threshold += 0.01f;

		if (alpha <= 0.0f) {
			threshold = 1.0f;
			alpha = 0.0f; // 最小値を上回らないようにする
			state_ = State::Inactive; // 非アクティブな状態に遷移
		}

		// スプライトのカラーにアルファ値を適用
		mask_->SetColor({0.0f, 0.0f, 0.0f, alpha});
		upDivider_->SetDissolveThreshold(threshold);
		underDivider_->SetDissolveThreshold(threshold);
	}
	break;
	default:

		break;
	}

	mask_->Update();
	upDivider_->Update();
	underDivider_->Update();
}

void TutorialDecoration::Start() {
	state_ = State::Start;
}

void TutorialDecoration::End() {
	state_ = State::End;
}
