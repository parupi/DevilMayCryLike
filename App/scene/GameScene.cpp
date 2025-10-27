#include <scene/GameScene.h>
#include <base/TextureManager.h>
#include <base/Particle/ParticleManager.h>
#include "debuger/ImGuiManager.h"
#include <math/Quaternion.h>
#include <math/Vector3.h>
#include <math/Matrix4x4.h>
#include <3d/Object/Model/ModelManager.h>
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <Include/SceneLoader.h>
#include <Include/SceneBuilder.h>
#include <3d/SkySystem/SkySystem.h>
#include <GameObject/Event/EventManager.h>
#include <scene/Transition/TransitionManager.h>
#include <scene/Transition/SceneTransitionController.h>
#include <2d/SpriteManager.h>


void GameScene::Initialize()
{
	// カメラの生成
	std::unique_ptr<GameCamera> gameCamera = std::make_unique<GameCamera>("GameCamera");
	gameCamera->GetTranslate() = { 0.096f, 13.4f, -20.0f };
	gameCamera->GetRotate() = { 0.5f, -0.005f, 0.0f };
	gameCamera_ = gameCamera.get();
	cameraManager_->AddCamera(std::move(gameCamera));

	stageStart_.Initialize();

	ModelManager::GetInstance()->LoadModel("PlayerBody");
	ModelManager::GetInstance()->LoadModel("PlayerHead");
	ModelManager::GetInstance()->LoadModel("PlayerLeftArm");
	ModelManager::GetInstance()->LoadModel("PlayerRightArm");
	ModelManager::GetInstance()->LoadModel("weapon");
	ModelManager::GetInstance()->LoadModel("Cube");
	ModelManager::GetInstance()->LoadModel("suzannu");
	ModelManager::GetInstance()->LoadModel("Sword");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("Terrain.png");
	TextureManager::GetInstance()->LoadTexture("gradationLine_brightened.png");
	TextureManager::GetInstance()->LoadTexture("MagicEffect.png");
	TextureManager::GetInstance()->LoadTexture("portal.png");

	// ステージの情報を読み込んで生成
	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Stage1.json"));

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("fire", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("smoke", "circle.png");

	// スカイボックスを生成
	SkySystem::GetInstance()->CreateSkyBox("moonless_golf_4k.dds");

	// ============ライト=================//
	lightManager_->CreateDirectionalLight("gameDir");

	player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
}

void GameScene::Finalize()
{
	Object3dManager::GetInstance()->DeleteAllObject();
	CameraManager::GetInstance()->DeleteAllCamera();
	LightManager::GetInstance()->DeleteAllLight();

	EventManager::GetInstance()->Finalize();
}

void GameScene::Update()
{
	stageStart_.Update();

	lightManager_->UpdateAllLight();

#ifdef _DEBUG
	DebugUpdate();
#endif
}

void GameScene::Draw()
{
	// 全オブジェクトの描画
	Object3dManager::GetInstance()->DrawSet();
	// スプライトの描画前処理
	SpriteManager::GetInstance()->DrawSet();
	// プレイヤーのスプライト描画
	if (player_) {
		player_->DrawEffect();
	}

	// 全パーティクルの描画
	ParticleManager::GetInstance()->Draw();
}

void GameScene::DrawRTV()
{
}

#ifdef _DEBUG
void GameScene::DebugUpdate()
{
	if (player_) {
		player_->DebugGui();
	}
}
#endif // _DEBUG
