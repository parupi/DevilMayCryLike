#include "TitleScene.h"
#include <base/TextureManager.h>
#include <2d/SpriteManager.h>
#include <input/Input.h>

void TitleScene::Initialize()
{
	TextureManager::GetInstance()->LoadTexture("resource/fade1x1.png");

	// カメラの生成

}

void TitleScene::Finalize()
{
}

void TitleScene::Update()
{
	cameraManager_->Update();

	fade_->Update();

	ChangePhase();
}

void TitleScene::Draw()
{
	//Object3dManager::GetInstance()->DrawSet();
	//fade_->Draw();

	SpriteManager::GetInstance()->DrawSet();
	fade_->DrawSprite();
}

void TitleScene::DrawRTV()
{
}

#ifdef _DEBUG
void TitleScene::DebugUpdate()
{
}
#endif // _DEBUG



void TitleScene::ChangePhase()
{
	switch (phase_) {
	case TitlePhase::kTitle:
		// キーボードの処理
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			phase_ = TitlePhase::kFadeOut;
			fade_->Start(Status::FadeOut, 2.0f);
		}
		break;
	case TitlePhase::kFadeIn:
		if (fade_->IsFinished()) {
			phase_ = TitlePhase::kTitle;
		}
		break;
	case TitlePhase::kFadeOut:
		if (fade_->IsFinished()) {
			// シーンの切り替え依頼
			SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		}
		break;
	}

}
