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

	OffScreenManager::GetInstance()->Initialize(dxManager.get(), psoManager.get(), srvManager.get());

	PrimitiveLineDrawer::GetInstance()->Initialize(dxManager.get(), psoManager.get(), srvManager.get());

	SkySystem::GetInstance()->Initialize(dxManager.get(), psoManager.get(), srvManager.get());

	RendererManager::GetInstance()->Initialize(dxManager.get(), srvManager.get());

	CollisionManager::GetInstance()->Initialize();

	CameraManager::GetInstance()->Initialize(dxManager.get());

	LightManager::GetInstance()->Initialize(dxManager.get());
	// 最初のシーンを生成
	sceneFactory_ = std::make_unique<SceneFactory>();
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->ChangeScene("TITLE");

	// インスタンス生成
	GlobalVariables::GetInstance();
}

void MyGameTitle::Finalize()
{
	// 描画処理系
	ImGuiManager::GetInstance()->Finalize();
	PrimitiveLineDrawer::GetInstance()->Finalize();
	RendererManager::GetInstance()->Finalize();
	SkySystem::GetInstance()->Finalize();

	// シーン
	SceneManager::GetInstance()->Finalize();

	// ゲームオブジェクト系
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
	ImGuiManager::GetInstance()->Begin();
	CameraManager::GetInstance()->Update();
	ParticleManager::GetInstance()->Update();
	GuchisFramework::Update();
	Object3dManager::GetInstance()->Update();
	RendererManager::GetInstance()->Update();
	CollisionManager::GetInstance()->Update();

	OffScreenManager::GetInstance()->Update();

	ImGuiManager::GetInstance()->End();
}

void MyGameTitle::Draw()
{
	OffScreenManager::GetInstance()->BeginDrawToPingPong();

	srvManager->BeginDraw();
	// プリミティブ描画前処理
	PrimitiveLineDrawer::GetInstance()->BeginDraw();
	// 天球やスカイボックスの描画
	SkySystem::GetInstance()->Draw();
	// シーン描画処理
	SceneManager::GetInstance()->Draw();

	OffScreenManager::GetInstance()->EndDrawToPingPong();

	dxManager->BeginDraw();
	//SceneManager::GetInstance()->DrawRTV();

	OffScreenManager::GetInstance()->DrawPostEffect();

#ifdef _DEBUG
	CollisionManager::GetInstance()->Draw();
#endif
	PrimitiveLineDrawer::GetInstance()->EndDraw();


	ImGuiManager::GetInstance()->Draw();

	dxManager->EndDraw();
}

