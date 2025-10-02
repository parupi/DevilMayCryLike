#include "TitleScene.h"
#include <base/TextureManager.h>
#include <2d/SpriteManager.h>
#include <input/Input.h>

void TitleScene::Initialize()
{
	TextureManager::GetInstance()->LoadTexture("white.png");
	TextureManager::GetInstance()->LoadTexture("StageClear.png");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("circle.png");

	// カメラの生成
	camera_ = std::make_unique<Camera>("TitleCamera");
	camera_->GetTranslate() = { 0.0f, 16.0f, -25.0f };
	camera_->GetRotate() = { 0.5f, 0.0f, 0.0f };
	cameraManager_->AddCamera(std::move(camera_));
	cameraManager_->SetActiveCamera(0);

	// タイトルシーンにあるもやもやを生成
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSmoke", "circle.png");

	smokeEmitter_ = std::make_unique<ParticleEmitter>();
	smokeEmitter_->Initialize("TitleSmoke");

	fade_ = std::make_unique<Fade>();
	fade_->Initialize();


	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("StageClear.png");
	sprite_->SetPosition({ 640.0f, 360.0f });
	sprite_->SetSize({ 1280.0f, 720.0f });

	sprite_->SetAnchorPoint({ 0.5f, 0.5f });
}

void TitleScene::Finalize()
{
	CameraManager::GetInstance()->DeleteAllCamera();
}

void TitleScene::Update()
{
	smokeEmitter_->Update();

	cameraManager_->Update();

	fade_->Update();

	sprite_->Update();

	ChangePhase();
}

void TitleScene::Draw()
{
	//Object3dManager::GetInstance()->DrawSet();
	//fade_->Draw();

	ParticleManager::GetInstance()->Draw();

	SpriteManager::GetInstance()->DrawSet();
	fade_->DrawSprite();

	//sprite_->Draw();
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
