#include "MyGameTitle.h"
#include <SceneFactory.h>
#include <Particle/ParticleManager.h>
#include "offscreen/OffScreenManager.h"
#include "Primitive/PrimitiveDrawer.h"
#include <Light/LightManager.h>
#include <CameraManager.h>
#include <Renderer/RendererManager.h>
#include <Collider/CollisionManager.h>

void MyGameTitle::Initialize()
{
	GuchisFramework::Initialize();
	// ImGui初期化
	ImGuiManager::GetInstance()->Initialize(winManager.get(), dxManager.get());
	// 2Dテクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dxManager.get(), srvManager.get());
	// 3Dテクスチャマネージャーの初期化
	ModelManager::GetInstance()->Initialize(dxManager.get(), srvManager.get());
	// パーティクルマネージャーの初期化
	ParticleManager::GetInstance()->Initialize(dxManager.get(), srvManager.get(), psoManager.get());
	// スプライト共通部の初期化
	SpriteManager::GetInstance()->Initialize(dxManager.get(), psoManager.get());
	// オブジェクト共通部
	Object3dManager::GetInstance()->Initialize(dxManager.get(), psoManager.get());

	OffScreenManager::GetInstance()->Initialize(dxManager.get(), psoManager.get());

	PrimitiveDrawer::GetInstance()->Initialize(dxManager.get(), psoManager.get(), srvManager.get());

	RendererManager::GetInstance()->Initialize(dxManager.get());

	CollisionManager::GetInstance()->Initialize();

	CameraManager::GetInstance()->Initialize(dxManager.get());

	LightManager::GetInstance()->Initialize(dxManager.get());
	// 最初のシーンを生成
	sceneFactory_ = std::make_unique<SceneFactory>();
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->ChangeScene("GAMEPLAY");

	// インスタンス生成
	GlobalVariables::GetInstance();
}

void MyGameTitle::Finalize()
{
	PrimitiveDrawer::GetInstance()->Finalize();
	RendererManager::GetInstance()->Finalize();
	CollisionManager::GetInstance()->Finalize();
	CameraManager::GetInstance()->Finalize();
	LightManager::GetInstance()->Finalize();
	ParticleManager::GetInstance()->Finalize();
	SpriteManager::GetInstance()->Finalize();
	Object3dManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	OffScreenManager::GetInstance()->Finalize();
	ImGuiManager::GetInstance()->Finalize();
	GuchisFramework::Finalize();
}

void MyGameTitle::Update()
{
	ImGuiManager::GetInstance()->Begin();
	CameraManager::GetInstance()->Update();
	ParticleManager::GetInstance()->Update();
	GuchisFramework::Update();
	Object3dManager::GetInstance()->Update();
	CollisionManager::GetInstance()->Update();

	OffScreenManager::GetInstance()->Update();
	GlobalVariables::GetInstance()->Update();

	ImGuiManager::GetInstance()->End();
}

void MyGameTitle::Draw()
{
	dxManager->BeginDrawForRenderTarget();
	srvManager->BeginDraw();
	PrimitiveDrawer::GetInstance()->BeginDraw();
	SceneManager::GetInstance()->Draw();



	dxManager->BeginDraw();

	//SceneManager::GetInstance()->DrawRTV();

	OffScreenManager::GetInstance()->Draw();

#ifdef _DEBUG
	CollisionManager::GetInstance()->Draw();
#endif
	PrimitiveDrawer::GetInstance()->EndDraw();


	ImGuiManager::GetInstance()->Draw();

	dxManager->EndDraw();
}

