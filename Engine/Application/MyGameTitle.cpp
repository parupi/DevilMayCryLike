#include "MyGameTitle.h"
#include <Scene/SceneFactory.h>
#include <GameObjectRegister.h>
#include <Graphics/Rendering/Particle/ParticleManager.h>
#include "Graphics/Rendering/PostEffect/OffScreenManager.h"
#include <World3D/Primitive/PrimitiveLineDrawer.h>
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Camera/CameraManager.h>
#include <World3D/Light/LightManager.h>
#include <Scene/SceneManager.h>
#include <Scene/Transition/TransitionManager.h>
#include <Scene/Transition/SceneTransitionController.h>
#include <Graphics/Rendering/Sky/SkySystem.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>

void MyGameTitle::Initialize() {
	GuchisFramework::Initialize();
#ifdef _DEBUG
	// ImGui初期化
	ImGuiManager::GetInstance().Initialize(winManager.get(), dxManager.get());
#endif // IMGUI
	// 2Dテクスチャマネージャーの初期化
	TextureManager::GetInstance().Initialize(dxManager.get());
	// 3Dテクスチャマネージャーの初期化
	ModelManager::GetInstance().Initialize(dxManager.get());
	// パーティクルマネージャーの初期化
	ParticleManager::GetInstance().Initialize(dxManager.get(), psoManager.get());
	// スプライト共通部の初期化
	SpriteManager::GetInstance().Initialize(dxManager.get(), psoManager.get());
	// オブジェクト共通部
	Object3dManager::GetInstance().Initialize(dxManager.get(), psoManager.get());

	OffScreenManager::GetInstance().Initialize(dxManager.get(), psoManager.get());

	PrimitiveLineDrawer::GetInstance().Initialize(dxManager.get(), psoManager.get());

	SkySystem::GetInstance().Initialize(dxManager.get(), psoManager.get());

	RendererManager::GetInstance().Initialize(dxManager.get(), psoManager.get());

	CollisionManager::GetInstance().Initialize();

	CameraManager::GetInstance().Initialize(dxManager.get());

	LightManager::GetInstance().Initialize(dxManager.get());
	// 全ゲームオブジェクトクラスをファクトリーに登録
	RegisterAllGameObjects();

	// 最初のシーンを生成
	sceneFactory_ = std::make_unique<SceneFactory>();
	SceneManager::GetInstance().SetSceneFactory(sceneFactory_.get());
	SceneManager::GetInstance().ChangeScene("TITLE");

	// EngineContext に全サービスを登録（GuchisFramework::Initialize でコアサービスは登録済み）
	ctx_.object3dManager = &Object3dManager::GetInstance();
	ctx_.lightManager = &LightManager::GetInstance();
	ctx_.cameraManager = &CameraManager::GetInstance();
	ctx_.skySystem = &SkySystem::GetInstance();
	ctx_.offScreenManager = &OffScreenManager::GetInstance();
	ctx_.sceneManager = &SceneManager::GetInstance();
	ctx_.spriteManager = &SpriteManager::GetInstance();
	ctx_.transitionManager = &TransitionManager::GetInstance();
	ctx_.collisionManager = &CollisionManager::GetInstance();
	ctx_.primitiveLineDrawer = &PrimitiveLineDrawer::GetInstance();
#ifdef _DEBUG
	ctx_.imGuiManager = &ImGuiManager::GetInstance();
#endif

	renderPipeline_ = std::make_unique<RenderPipeline>();
	renderPipeline_->Initialize(ctx_);

}

void MyGameTitle::Finalize() {
	// 描画処理系
#ifdef _DEBUG
	ImGuiManager::GetInstance().Finalize();
#endif // DEBUG
	PrimitiveLineDrawer::GetInstance().Finalize();

	SkySystem::GetInstance().Finalize();

	// シーン
	SceneManager::GetInstance().Finalize();

	// ゲームオブジェクト系
	TransitionManager::GetInstance().Finalize();
	SceneTransitionController::GetInstance().Finalize();
	ParticleManager::GetInstance().Finalize();
	SpriteManager::GetInstance().Finalize();
	Object3dManager::GetInstance().Finalize();
	ModelManager::GetInstance().Finalize();

	// 基盤系
	RendererManager::GetInstance().Finalize();
	CollisionManager::GetInstance().Finalize();
	CameraManager::GetInstance().Finalize();
	LightManager::GetInstance().Finalize();
	TextureManager::GetInstance().Finalize();
	OffScreenManager::GetInstance().Finalize();
	// 各種描画パスの削除
	renderPipeline_->Finalize();
	// フレームワークベース
	GuchisFramework::Finalize();
}

void MyGameTitle::Update() {
#ifdef _DEBUG
	ImGuiManager::GetInstance().Begin();
#endif // DEBUG
	GuchisFramework::Update();
	CameraManager::GetInstance().Update();
	ParticleManager::GetInstance().Update();
	SceneTransitionController::GetInstance().Update();
	Object3dManager::GetInstance().Update();
	RendererManager::GetInstance().Update();
	CollisionManager::GetInstance().Update();

	LightManager::GetInstance().Update();
	OffScreenManager::GetInstance().Update();
#ifdef _DEBUG
	SceneManager::GetInstance().DebugUpdate();
	ImGuiManager::GetInstance().End();
#endif // DEBUG
}

void MyGameTitle::Draw() {
	renderPipeline_->Execute();
}

void MyGameTitle::RemoveObjects() {
	RendererManager::GetInstance().RemoveDeadObjects();
	CollisionManager::GetInstance().RemoveDeadObjects();
	Object3dManager::GetInstance().RemoveDeadObject();
}

