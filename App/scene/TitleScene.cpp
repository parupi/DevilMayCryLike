#include "TitleScene.h"
#include <base/TextureManager.h>
#include <2d/SpriteManager.h>
#include <input/Input.h>

void TitleScene::Initialize()
{
	TextureManager::GetInstance()->LoadTexture("white.png");
	TextureManager::GetInstance()->LoadTexture("StageClear.png");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");

	// カメラの生成


	fade_ = std::make_unique<Fade>();
	fade_->Initialize();

	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("StageClear.png");
	//sprite_->SetPosition({ 630, 360 });
	sprite_->SetSize({ 1280, 720 });
	//sprite_->SetAnchorPoint({ 0.5f, 0.5f });
}

void TitleScene::Finalize()
{
}

void TitleScene::Update()
{
	cameraManager_->Update();

	fade_->Update();

	sprite_->Update();

	ChangePhase();
}

void TitleScene::Draw()
{
	//Object3dManager::GetInstance()->DrawSet();
	//fade_->Draw();

	SpriteManager::GetInstance()->DrawSet();
	fade_->DrawSprite();

	sprite_->Draw();
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
			fade_->Start(Status::FadeOut, 1.0f);
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
