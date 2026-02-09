#include "GameScene.h"
#include "base/TextureManager.h"
#include "base/Particle/ParticleManager.h"
#include "debuger/ImGuiManager.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"
#include "math/Matrix4x4.h"
#include "3d/Object/Model/ModelManager.h"
#include "3d/Object/Renderer/RendererManager.h"
#include "3d/Object/Renderer/ModelRenderer.h"
#include "3d/Collider/CollisionManager.h"
#include "3d/Object/Renderer/PrimitiveRenderer.h"
#include "Include/SceneLoader.h"
#include "Include/SceneBuilder.h"
#include "3d/SkySystem/SkySystem.h"
#include "GameObject/Event/EventManager.h"
#include "scene/Transition/TransitionManager.h"
#include "scene/Transition/SceneTransitionController.h"
#include "2d/SpriteManager.h"
#include "GameObject/Camera/LockOnCamera.h"
#include "scene/GameScene/State/GameSceneStatePlay.h"
#include "State/GameSceneStateMenu.h"
#include "State/GameSceneStateStart.h"

void GameScene::Initialize()
{
	// ステートの生成
	states_["Start"] = std::make_unique<GameSceneStateStart>();
	states_["Play"] = std::make_unique<GameSceneStatePlay>();
	states_["Menu"] = std::make_unique<GameSceneStateMenu>();
	currentState_ = states_["Start"].get();

	// 入力の受付状態を管理するクラス生成
	inputContext_ = std::make_unique<InputContext>();
	inputContext_->Initialize(Input::GetInstance());

	// 最初のシーンに入る処理
	currentState_->Enter(*this);

	// カメラの生成
	std::unique_ptr<GameCamera> camera = std::make_unique<GameCamera>("GameCamera");
	camera->GetTranslate() = { 0.096f, 13.4f, -20.0f };
	camera->GetRotate() = { 0.5f, -0.005f, 0.0f };
	gameCamera_ = camera.get();
	cameraManager_->AddCamera(std::move(camera));

	std::unique_ptr<LockOnCamera> lockOnCamera = std::make_unique<LockOnCamera>("LockOnCamera");
	cameraManager_->AddCamera(std::move(lockOnCamera));

	// カメラの生成
	std::unique_ptr<ClearCamera> clearCamera = std::make_unique<ClearCamera>("ClearCamera");
	clearCamera_ = clearCamera.get();
	cameraManager_->AddCamera(std::move(clearCamera));

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
	TextureManager::GetInstance()->LoadTexture("DeathText.png");
	TextureManager::GetInstance()->LoadTexture("GameUI.png");
	TextureManager::GetInstance()->LoadTexture("reticle.png");
	TextureManager::GetInstance()->LoadTexture("hitSmoke.png");
	TextureManager::GetInstance()->LoadTexture("UI/Arrow.png");
	TextureManager::GetInstance()->LoadTexture("UI/plus.png");
	TextureManager::GetInstance()->LoadTexture("UI/RBButton.png");
	TextureManager::GetInstance()->LoadTexture("UI/SticDown.png");
	TextureManager::GetInstance()->LoadTexture("UI/YButton.png");

	// ステージの情報を読み込んで生成
	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Stage1.json"));

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("fire", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("smoke", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("hitSmoke", "hitSmoke.png");
	ParticleManager::GetInstance()->CreateParticleGroup("GameCircle", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("GameSmoke", "smoke.png");

	// スカイボックスを生成
	SkySystem::GetInstance()->CreateSkyBox("moonless_golf_4k.dds");

	// ============ライト=================//
	lightManager_->CreateDirectionalLight("gameDir");

	player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
	player_->SetInput(inputContext_->GetPlayerInput());

	gameUI_ = std::make_unique<GameUI>();
	gameUI_->Initialize();

	musk_ = std::make_unique<Sprite>();
	musk_->Initialize("white.png");
	musk_->SetSize({ 1280.0f, 720.0f });
	musk_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	menuUI_ = std::make_unique<MenuUI>();
	menuUI_->Initialize(this);


}

void GameScene::Finalize()
{
	Object3dManager::GetInstance()->DeleteAllObject();
	CollisionManager::GetInstance()->DeleteAllCollider();
	RendererManager::GetInstance()->DeleteAllRenderer();
	CameraManager::GetInstance()->DeleteAllCamera();
	LightManager::GetInstance()->DeleteAllLight();

	EventManager::GetInstance()->Finalize();
}

void GameScene::Update()
{
	lightManager_->UpdateAllLight();
	gameUI_->Update();

	if (currentState_) {
		currentState_->Update(*this);
	}

	if (currentState_ == states_["Menu"].get()) {
		player_->SetMenu(true);
	} else {
		player_->SetMenu(false);
	}

	musk_->SetColor({ 0.0f, 0.0f, 0.0f, muskAlpha_ });
	musk_->Update();

	menuUI_->Update();

	inputContext_->Update();

	Object3dManager::GetInstance()->SetDeltaTime(sceneTime_);

#ifdef _DEBUG
	//DebugUpdate();
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

	gameUI_->Draw();

	musk_->Draw();

	menuUI_->Draw();

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

	//Object3dManager::GetInstance()->FindObject("HellKaina")->DebugGui();
}
#endif // _DEBUG

void GameScene::ChangeState(const std::string& stateName)
{
	currentState_->Exit(*this);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(*this);
	}
}