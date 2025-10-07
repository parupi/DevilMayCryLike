#include "TitleScene.h"
#include <base/TextureManager.h>
#include <2d/SpriteManager.h>
#include <input/Input.h>
#include <Include/SceneLoader.h>
#include <Include/SceneBuilder.h>
#include <3d/Object/Model/ModelManager.h>
#include <3d/SkySystem/SkySystem.h>
#include <3d/Light/PointLight.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include <3d/Object/Renderer/RendererManager.h>

void TitleScene::Initialize()
{
	ModelManager::GetInstance()->LoadModel("PlayerBody");
	ModelManager::GetInstance()->LoadModel("PlayerHead");
	ModelManager::GetInstance()->LoadModel("weapon");
	ModelManager::GetInstance()->LoadModel("Sword");
	ModelManager::GetInstance()->LoadModel("Cube");
	TextureManager::GetInstance()->LoadTexture("white.png");
	TextureManager::GetInstance()->LoadTexture("Title.png");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("circle.png");
	TextureManager::GetInstance()->LoadTexture("circle2.png");
	TextureManager::GetInstance()->LoadTexture("TitleUnder.png");
	TextureManager::GetInstance()->LoadTexture("TitleUp.png");
	TextureManager::GetInstance()->LoadTexture("SelectArrow.png");
	TextureManager::GetInstance()->LoadTexture("GameStart.png");
	TextureManager::GetInstance()->LoadTexture("black.png");
	TextureManager::GetInstance()->LoadTexture("SelectMask.png");

	// カメラの生成
	camera_ = std::make_unique<Camera>("TitleCamera");
	camera_->GetTranslate() = { 0.0f, 5.2f, -25.0f };
	camera_->GetRotate() = { 0.25f, 0.0f, 0.0f };
	cameraManager_->AddCamera(std::move(camera_));
	cameraManager_->SetActiveCamera(0);

	// タイトルシーンにあるもやもやを生成
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSphere", "circle2.png");
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSmoke", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSmoke2", "smoke.png");

	// スカイボックスを生成
	SkySystem::GetInstance()->CreateSkyBox("qwantani_moon_noon_puresky_4k.dds");

	std::unique_ptr<PointLight> pointLight2 = std::make_unique<PointLight>("Point2");
	pointLight2->GetLightData().enabled = true;
	pointLight2->GetLightData().position = { 0.0f, 2.0f, -6.0f };
	pointLight2->GetLightData().color = { 0.3f, 0.3f, 0.3f , 0.5f };
	pointLight2->GetLightData().intensity = 2.0f;
	pointLight2->GetLightData().radius = 10.0f;
	pointLight2->GetLightData().decay = 1.0f;
	lightManager_->AddPointLight(std::move(pointLight2));

	std::unique_ptr<DirectionalLight> dirLight = std::make_unique<DirectionalLight>("Dir1");
	dirLight->GetLightData().enabled = true;
	dirLight->GetLightData().direction = { 0.0f, -1.0f, 0.0f };
	dirLight->GetLightData().color = { 1.0f, 1.0f, 1.0f , 1.0f };
	dirLight->GetLightData().intensity = 0.1f;
	lightManager_->AddDirectionalLight(std::move(dirLight));

	smokeEmitter_ = std::make_unique<ParticleEmitter>();
	smokeEmitter_->Initialize("TitleSmoke");

	smokeEmitter2_ = std::make_unique<ParticleEmitter>();
	smokeEmitter2_->Initialize("TitleSmoke2");

	sphereEmitter_ = std::make_unique<ParticleEmitter>();
	sphereEmitter_->Initialize("TitleSphere");

	fade_ = std::make_unique<Fade>();
	fade_->Initialize();
	fade_->Start(Status::FadeIn, 1.0f);

	phase_ = TitlePhase::kFadeIn;

	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Title.json"));

	titleWord_ = std::make_unique<Sprite>();
	titleWord_->Initialize("Title.png");
	titleWord_->SetPosition({ 640.0f, 156.0f });
	titleWord_->SetSize({ 576.0f, 192.0f });
	titleWord_->SetAnchorPoint({ 0.5f, 0.5f });

	titleUnder_ = std::make_unique<Sprite>();
	titleUnder_->Initialize("TitleUnder.png");
	titleUnder_->SetPosition({ 640.0f, 320.0f });
	titleUnder_->SetSize({ 576.0f, 192.0f });
	titleUnder_->SetAnchorPoint({ 0.5f, 0.5f });

	titleUp_ = std::make_unique<Sprite>();
	titleUp_->Initialize("TitleUp.png");
	titleUp_->SetPosition({ 640.0f, 16.0f });
	titleUp_->SetSize({ 576.0f, 192.0f });
	titleUp_->SetAnchorPoint({ 0.5f, 0.5f });

	for (int32_t i = 0; i < 2; i++) {
		selectArrows_[i] = std::make_unique<Sprite>();
		selectArrows_[i]->Initialize("SelectArrow.png");
		selectArrows_[i]->SetAnchorPoint({ 0.5f, 0.5f });

		if (i == 0) {
			selectArrows_[i]->SetPosition({ 530.0f, 520.0f });
		} else {
			selectArrows_[i]->SetSize({ -32.0f, 32.0f });
			selectArrows_[i]->SetPosition({ 750.0f, 520.0f });
		}
	}

	gameStart_ = std::make_unique<Sprite>();
	gameStart_->Initialize("GameStart.png");
	gameStart_->SetPosition({ 640.0f, 520.0f });
	gameStart_->SetAnchorPoint({ 0.5f, 0.5f });

	selectMask_ = std::make_unique<Sprite>();
	selectMask_->Initialize("circle.png");
	selectMask_->SetPosition({ 640.0f, 520.0f });
	selectMask_->SetSize({ 500.0f, 100.0f });
	selectMask_->SetAnchorPoint({ 0.5f, 0.5f });
	selectMask_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f});

	// プレイヤーの生成
	std::unique_ptr<Object3d> playerObject = std::make_unique<Object3d>("Player");
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("Player", "PlayerHead"));
	playerObject->AddRenderer(RendererManager::GetInstance()->FindRender("Player"));
	playerObject->GetWorldTransform()->GetTranslation() = { 0.0f, 0.0f, -7.0f };
	Object3dManager::GetInstance()->AddObject(std::move(playerObject));

	// 武器の生成
	std::unique_ptr<Object3d> weaponObject = std::make_unique<Object3d>("Sword");
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("Sword", "Sword"));
	weaponObject->AddRenderer(RendererManager::GetInstance()->FindRender("Sword"));
	weaponObject->GetWorldTransform()->GetTranslation() = { 0.0f, 0.75f, -7.5f };
	weaponObject->GetWorldTransform()->GetScale() = { 0.5f, 0.5f, 0.5f };
	// 回転の計算
	Vector3 direction = { 135.0f, 90.0f, 0.0f };
	weaponObject->GetWorldTransform()->GetRotation() = EulerDegree(direction);

	weaponObject_ = weaponObject.get();
	Object3dManager::GetInstance()->AddObject(std::move(weaponObject));
}

void TitleScene::Finalize()
{
	Object3dManager::GetInstance()->DeleteAllObject();
	CameraManager::GetInstance()->DeleteAllCamera();
	LightManager::GetInstance()->DeleteAllLight();
}

void TitleScene::Update()
{
	smokeEmitter_->Update();
	smokeEmitter2_->Update();

	sphereEmitter_->Update();

	//cameraManager_->Update();

	lightManager_->UpdateAllLight();

	fade_->Update();

	titleWord_->Update();
	titleUnder_->Update();
	titleUp_->Update(); 
	gameStart_->Update();

	for (auto& arrow : selectArrows_) {
		arrow->Update();
	}

	selectMask_->Update();

	ChangePhase();


#ifdef _DEBUG
	DebugUpdate();
#endif // _DEBUG

}

void TitleScene::Draw()
{
	Object3dManager::GetInstance()->DrawSet();
	//fade_->Draw();

	ParticleManager::GetInstance()->Draw();

	SpriteManager::GetInstance()->DrawSet();
	titleWord_->Draw();
	titleUnder_->Draw();
	titleUp_->Draw();
	gameStart_->Draw();

	for (auto& arrow : selectArrows_) {
		arrow->Draw();
	}

	SpriteManager::GetInstance()->DrawSet(BlendMode::kAdd);
	selectMask_->Draw();

	SpriteManager::GetInstance()->DrawSet(BlendMode::kNormal);
	fade_->DrawSprite();
}

void TitleScene::DrawRTV()
{
}

#ifdef _DEBUG
void TitleScene::DebugUpdate()
{
	ImGui::Begin("weapon");
	weaponObject_->DebugGui();
	ImGui::End();
}
#endif // _DEBUG

void TitleScene::ChangePhase()
{
	switch (phase_) {
	case TitlePhase::kTitle:
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			phase_ = TitlePhase::kUIAnimation;
			uiAnimationTimer_ = 0.0f; // リセット
		}
		break;

	case TitlePhase::kUIAnimation: {
		uiAnimationTimer_ += DeltaTime::GetDeltaTime(); // 経過時間を加算（DeltaTime関数などがある前提）

		// ★UIをアニメーション
		float t = std::clamp(uiAnimationTimer_ / uiAnimationDuration_, 0.0f, 1.0f);

		// 例：透明度を下げる or サイズを縮小する
		selectMask_->SetColor({ 1.0f, 1.0f, 1.0f, t });

		for (auto& arrow : selectArrows_) {
			arrow->SetSize({ 1.0f - 0.3f * t, 1.0f - 0.3f * t });
		}

		if (uiAnimationTimer_ >= uiAnimationDuration_) {
			// アニメーションが終わったらフェード開始
			phase_ = TitlePhase::kFadeOut;
			fade_->Start(Status::FadeOut, 1.0f);
		}
		break;
	}
	case TitlePhase::kFadeIn:
		if (fade_->IsFinished()) {
			phase_ = TitlePhase::kTitle;
		}
		break;

	case TitlePhase::kFadeOut:
		if (fade_->IsFinished()) {
			SceneManager::GetInstance()->ChangeScene("TITLE");
		}
		break;
	}
}
