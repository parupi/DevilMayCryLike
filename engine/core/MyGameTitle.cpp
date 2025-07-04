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

	PrimitiveLineDrawer::GetInstance()->Initialize(dxManager.get(), psoManager.get(), srvManager.get());

	RendererManager::GetInstance()->Initialize(dxManager.get());

	CollisionManager::GetInstance()->Initialize();

	CameraManager::GetInstance()->Initialize(dxManager.get());

	LightManager::GetInstance()->Initialize(dxManager.get());
	// 最初のシーンを生成
	sceneFactory_ = std::make_unique<SceneFactory>();
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->ChangeScene("SAMPLE");
	// キューブマップの生成
	skySystem_ = std::make_unique<SkySystem>();
	skySystem_->Initialize(dxManager.get(), psoManager.get(), srvManager.get());

	// インスタンス生成
	GlobalVariables::GetInstance();
}

void MyGameTitle::Finalize()
{
	// 描画処理系
	ImGuiManager::GetInstance()->Finalize();               // 最もUI描画の最後に使うので最初に破棄
	PrimitiveLineDrawer::GetInstance()->Finalize();        // モデル等の描画で使っているかも
	RendererManager::GetInstance()->Finalize();            // SpriteやModelに使われる

	// ゲームオブジェクト系
	ParticleManager::GetInstance()->Finalize();            // TextureやRendererに依存
	SpriteManager::GetInstance()->Finalize();              // Textureに依存
	Object3dManager::GetInstance()->Finalize();            // ModelManagerやCollisionに依存
	ModelManager::GetInstance()->Finalize();               // Textureに依存

	// 基盤系
	CollisionManager::GetInstance()->Finalize();
	CameraManager::GetInstance()->Finalize();
	LightManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();             // 多くの描画系に依存される
	OffScreenManager::GetInstance()->Finalize();           // RTV/DSV/Texture使っている可能性がある
	// フレームワークベース
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

	ImGuiManager::GetInstance()->End();
}

void MyGameTitle::Draw()
{
	dxManager->BeginDrawForRenderTarget();
	srvManager->BeginDraw();
	PrimitiveLineDrawer::GetInstance()->BeginDraw();
	SceneManager::GetInstance()->Draw();

	skySystem_->Draw();

	dxManager->BeginDraw();

	//SceneManager::GetInstance()->DrawRTV();

	OffScreenManager::GetInstance()->Draw();

#ifdef _DEBUG
	CollisionManager::GetInstance()->Draw();
#endif
	PrimitiveLineDrawer::GetInstance()->EndDraw();


	ImGuiManager::GetInstance()->Draw();

	dxManager->EndDraw();
}

