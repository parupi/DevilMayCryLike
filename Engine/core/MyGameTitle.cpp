#include "MyGameTitle.h"
#include <scene/SceneFactory.h>
#include <base/Particle/ParticleManager.h>
#include "offscreen/OffScreenManager.h"
#include <3d/Primitive/PrimitiveLineDrawer.h>
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Camera/CameraManager.h>
#include <3d/Light/LightManager.h>
#include <scene/SceneManager.h>
#include <scene/Transition/TransitionManager.h>
#include <scene/Transition/SceneTransitionController.h>

void MyGameTitle::Initialize()
{
	GuchisFramework::Initialize();
#ifdef _DEBUG
	// ImGui初期化
	ImGuiManager::GetInstance()->Initialize(winManager.get(), dxManager.get());
#endif // DEBUG
	// 2Dテクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dxManager.get());
	// 3Dテクスチャマネージャーの初期化
	ModelManager::GetInstance()->Initialize(dxManager.get());
	// パーティクルマネージャーの初期化
	ParticleManager::GetInstance()->Initialize(dxManager.get(), psoManager.get());
	// スプライト共通部の初期化
	SpriteManager::GetInstance()->Initialize(dxManager.get(), psoManager.get());
	// オブジェクト共通部
	Object3dManager::GetInstance()->Initialize(dxManager.get(), psoManager.get());

	OffScreenManager::GetInstance()->Initialize(dxManager.get(), psoManager.get());

	PrimitiveLineDrawer::GetInstance()->Initialize(dxManager.get(), psoManager.get());

	SkySystem::GetInstance()->Initialize(dxManager.get(), psoManager.get());

	RendererManager::GetInstance()->Initialize(dxManager.get(), psoManager.get());

	CollisionManager::GetInstance()->Initialize();

	CameraManager::GetInstance()->Initialize(dxManager.get());

	LightManager::GetInstance()->Initialize(dxManager.get());
	// 最初のシーンを生成
	sceneFactory_ = std::make_unique<SceneFactory>();
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->ChangeScene("SAMPLE");

	gBufferPath = std::make_unique<GBufferPath>();
	gBufferPath->Initialize(dxManager.get(), gBufferManager.get());

	lightingPath = std::make_unique<LightingPath>();
	lightingPath->Initialize(dxManager.get(), gBufferManager.get(), psoManager.get());

	// インスタンス生成
	GlobalVariables::GetInstance();
}

void MyGameTitle::Finalize()
{
	// 描画処理系
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Finalize();
#endif // DEBUG
	PrimitiveLineDrawer::GetInstance()->Finalize();
	RendererManager::GetInstance()->Finalize();
	SkySystem::GetInstance()->Finalize();

	// シーン
	SceneManager::GetInstance()->Finalize();

	// ゲームオブジェクト系
	TransitionManager::GetInstance()->Finalize();
	SceneTransitionController::GetInstance()->Finalize();
	ParticleManager::GetInstance()->Finalize(); 
	SpriteManager::GetInstance()->Finalize();           
	Object3dManager::GetInstance()->Finalize();         
	ModelManager::GetInstance()->Finalize();            

	// 基盤系
	CollisionManager::GetInstance()->Finalize();
	CameraManager::GetInstance()->Finalize();
	LightManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();          
	OffScreenManager::GetInstance()->Finalize();    
	// フレームワークベース
	GuchisFramework::Finalize();
}

void MyGameTitle::Update()
{
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Begin();
#endif // DEBUG
	CameraManager::GetInstance()->Update();
	ParticleManager::GetInstance()->Update();
	GuchisFramework::Update();
	SceneTransitionController::GetInstance()->Update();
	Object3dManager::GetInstance()->Update();
	RendererManager::GetInstance()->Update();
	CollisionManager::GetInstance()->Update();

	OffScreenManager::GetInstance()->Update();
#ifdef _DEBUG
	ImGuiManager::GetInstance()->End();
#endif // DEBUG
}

void MyGameTitle::Draw()
{
	OffScreenManager::GetInstance()->BeginDrawToPingPong();

	dxManager->GetSrvManager()->BeginDraw();
	// プリミティブ描画前処理
	//PrimitiveLineDrawer::GetInstance()->BeginDraw();


	OffScreenManager::GetInstance()->EndDrawToPingPong();

	dxManager->BeginDraw();

	OffScreenManager::GetInstance()->DrawPostEffect();



	gBufferPath->Begin();


	// 天球やスカイボックスの描画
	SkySystem::GetInstance()->Draw();
	// シーン描画処理
	SceneManager::GetInstance()->Draw();
	// シーンの描画が終わった後にトランジションの描画
	SceneTransitionController::GetInstance()->Draw();
	//CollisionManager::GetInstance()->Draw();

	//PrimitiveLineDrawer::GetInstance()->EndDraw();

	gBufferPath->End();

	lightingPath->Begin();

	LightManager::GetInstance()->BindLightsForDeferred();

	lightingPath->DrawDirectionalLight();
	lightingPath->End();

#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif // DEBUG

	dxManager->EndDraw();
}

void MyGameTitle::RemoveObjects()
{
	RendererManager::GetInstance()->RemoveDeadObjects();
	CollisionManager::GetInstance()->RemoveDeadObjects();
	Object3dManager::GetInstance()->RemoveDeadObject();
}

