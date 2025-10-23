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
#include <scene/Transition/FadeTransition.h>
#include <scene/Transition/TransitionManager.h>
#include <GameObject/Camera/TitleCamera.h>


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
	
	cameraManager_->AddCamera(std::make_unique<TitleCamera>("TitleCamera"));
	cameraManager_->SetActiveCamera(0);

	camera_ = static_cast<TitleCamera*>(cameraManager_->GetActiveCamera());
	camera_->Initialize();
	camera_->Enter();

	// タイトルシーンにあるもやもやを生成
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSphere", "circle2.png");
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSmoke", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSmoke2", "smoke.png");

	// スカイボックスを生成
	SkySystem::GetInstance()->CreateSkyBox("qwantani_moon_noon_puresky_4k.dds");

	lightManager_->CreatePointLight("TitlePoint");
	lightManager_->CreateDirectionalLight("TitleDir");
	lightManager_->CreateSpotLight("TitleSpot");

	smokeEmitter_ = std::make_unique<ParticleEmitter>();
	smokeEmitter_->Initialize("TitleSmoke");

	smokeEmitter2_ = std::make_unique<ParticleEmitter>();
	smokeEmitter2_->Initialize("TitleSmoke2");

	sphereEmitter_ = std::make_unique<ParticleEmitter>();
	sphereEmitter_->Initialize("TitleSphere");

	phase_ = TitlePhase::kFadeIn;

	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Title.json"));

	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Title", PrimitiveType::Plane, "Title.png"));

	std::unique_ptr<PrimitiveRenderer> primitive = std::make_unique<PrimitiveRenderer>("TitleUp", PrimitiveType::Plane, "TitleUp.png");
	primitive->GetWorldTransform()->GetTranslation() = { 0.0f, 0.0f, 0.7f };
	RendererManager::GetInstance()->AddRenderer(std::move(primitive));

	primitive = std::make_unique<PrimitiveRenderer>("TitleUnder", PrimitiveType::Plane, "TitleUnder.png");
	primitive->GetWorldTransform()->GetTranslation() = { 0.0f, 0.0f, -0.8f };
	RendererManager::GetInstance()->AddRenderer(std::move(primitive));

	std::unique_ptr<Object3d> object = std::make_unique<Object3d>("Title");
	object->Initialize();
	object->AddRenderer(RendererManager::GetInstance()->FindRender("Title"));
	object->AddRenderer(RendererManager::GetInstance()->FindRender("TitleUp"));
	object->AddRenderer(RendererManager::GetInstance()->FindRender("TitleUnder"));

	object->GetWorldTransform()->GetTranslation() = { 0.0f, 3.2f, -7.0f };
	object->GetWorldTransform()->GetScale() = { 6.0f, 1.0f, 2.0f };

	Vector3 dir = { -90.0f, 0.0f, 0.0f };
	object->GetWorldTransform()->GetRotation() = EulerDegree(dir);

	Object3dManager::GetInstance()->AddObject(std::move(object));

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
	selectMask_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

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

	// 新しいトランジションの追加 // 追加したら勝手に設定してくれる
	TransitionManager::GetInstance()->AddTransition(std::make_unique<FadeTransition>("Fade"));
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

	lightManager_->UpdateAllLight();

	gameStart_->Update();

	for (auto& arrow : selectArrows_) {
		arrow->Update();
	}

	selectMask_->Update();

	ExitUpdate();
	ChangePhase();

#ifdef _DEBUG
	DebugUpdate();
#endif // _DEBUG

}

void TitleScene::Draw()
{
	Object3dManager::GetInstance()->DrawSet();

	ParticleManager::GetInstance()->Draw();

	SpriteManager::GetInstance()->DrawSet();
	gameStart_->Draw();

	for (auto& arrow : selectArrows_) {
		arrow->Draw();
	}

	SpriteManager::GetInstance()->DrawSet(BlendMode::kAdd);
	selectMask_->Draw();
}

void TitleScene::DrawRTV()
{
}

void TitleScene::Exit()
{
	isExit_ = true;

	targetArrowSizes_[0] = { -32.0f, 32.0f };
	targetArrowSizes_[1] = { 32.0f, 32.0f };
	targetSpriteAlpha_ = 0.0f;
	targetSelectMaskAlpha = 0.0f;

	for (size_t i = 0; i < selectArrows_.size(); i++) {
		startArrowSizes_[i] = selectArrows_[i]->GetSize();
	}

	startSpriteAlpha_ = 1.0f;
	startSelectMaskAlpha = 0.0f;
}

void TitleScene::ExitUpdate()
{
	if (!isExit_) return;

	exitTimer_ += DeltaTime::GetDeltaTime();
	// 0.0f ～ 1.0f にクランプ
	float t = std::clamp(exitTimer_ / exitTime_, 0.0f, 1.0f);

	// --- GameStart スプライトのアルファ補間 ---
	float spriteAlpha = Lerp(startSpriteAlpha_, targetSpriteAlpha_, t);
	gameStart_->SetColor({ 1.0f, 1.0f, 1.0f, spriteAlpha });

	// --- マスク透明度（前半フェードイン、後半フェードアウト） ---
	float maskAlpha = 0.0f;
	if (t < 0.5f) {
		float subT = t / 0.5f; // 0.0～1.0
		maskAlpha = Lerp(startSelectMaskAlpha, 0.8f, subT);
	} else {
		float subT = (t - 0.5f) / 0.5f; // 0.0～1.0
		maskAlpha = Lerp(0.8f, targetSelectMaskAlpha, subT);
	}
	selectMask_->SetColor({ 1.0f, 1.0f, 1.0f, maskAlpha });

	// --- 矢印のサイズ補間 ---
	for (size_t i = 0; i < selectArrows_.size(); i++) {
		Vector2 size;
		size.x = Lerp(startArrowSizes_[i].x, targetArrowSizes_[i].x, t);
		size.y = Lerp(startArrowSizes_[i].y, targetArrowSizes_[i].y, t);
		selectArrows_[i]->SetSize(size);
		selectArrows_[i]->SetColor({ 1.0f, 1.0f, 1.0f, spriteAlpha });
	}
}

#ifdef _DEBUG
void TitleScene::DebugUpdate()
{
	ImGui::Begin("weapon");
	weaponObject_->DebugGui();
	ImGui::End();

	ImGui::Begin("title");
	Object3dManager::GetInstance()->FindObject("Title")->DebugGui();
	ImGui::End();
}
#endif // _DEBUG

void TitleScene::ChangePhase()
{
	if (!camera_->IsExit()) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			camera_->Exit();
			Exit();
		}
	}
}
