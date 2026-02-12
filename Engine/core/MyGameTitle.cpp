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
	gBufferPath->Initialize(dxManager.get(), gBufferManager.get(), psoManager.get());

	lightingPath = std::make_unique<LightingPath>();
	lightingPath->Initialize(dxManager.get(), gBufferManager.get(), psoManager.get());

	forwardPath = std::make_unique<ForwardRenderPath>();
	forwardPath->Initialize(dxManager.get(), psoManager.get());

	compositePath = std::make_unique<CompositePath>();
	compositePath->Initialize(dxManager.get(), psoManager.get());

	csm = std::make_unique<CascadedShadowMap>();
	csm->Initialize(dxManager.get(), 1280);

	shadowPath = std::make_unique<ShadowPass>();
	shadowPath->Initialize(dxManager.get(), psoManager.get(), csm.get());

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
	csm->Update();
#ifdef _DEBUG
	ImGuiManager::GetInstance()->End();
#endif // DEBUG
}

void MyGameTitle::Draw()
{
	shadowPath->BeginDraw();
	shadowPath->Execute();
	shadowPath->EndDraw();

	///---------------------------------------------------------
	/// GBufferPath（Deferredの各バッファ生成）
	///---------------------------------------------------------
	gBufferPath->Begin();

	Object3dManager::GetInstance()->DrawDeferred();

	gBufferPath->End();

	///---------------------------------------------------------
	/// LightingPath（GBuffer結果を使って描画）
	///---------------------------------------------------------
	lightingPath->Begin();

	LightManager::GetInstance()->BindLightsToShader();
	CameraManager::GetInstance()->BindCameraToShader();

	lightingPath->End();

	///---------------------------------------------------------
	/// ForwardRenderPath
	///---------------------------------------------------------

	// 描画前処理
	forwardPath->BeginDraw();
	// スカイボックスの描画
	SkySystem::GetInstance()->Draw();
	// Forward描画で設定されているオブジェクトの描画
	Object3dManager::GetInstance()->DrawForward();
	// シーンの描画
	SceneManager::GetInstance()->Draw();
	// トランジションの描画
	TransitionManager::GetInstance()->Draw();
	// 描画後処理
	forwardPath->EndDraw();

	///---------------------------------------------------------
	/// CompositePath (deferred結果とforward結果の合成)
	///---------------------------------------------------------

	compositePath->Composite(forwardPath->GetSrvIndex(), forwardPath->GetSrvForDepthIndex(), lightingPath->GetOutputSrvIndex(), gBufferManager->GetDepthIndex());

	///---------------------------------------------------------
	/// OffScreen（ポストエフェクト前の下準備 or 前景エフェクト）
	///---------------------------------------------------------
	OffScreenManager::GetInstance()->CopyLightingToPing(compositePath->GetSrvIndex());

	OffScreenManager::GetInstance()->BeginDrawToPingPong();

	OffScreenManager::GetInstance()->EndDrawToPingPong();

	///---------------------------------------------------------
	/// PostEffectPath（Ping-Pong結果から最終1枚に統合）
	///---------------------------------------------------------
	OffScreenManager::GetInstance()->ExecutePostEffects();


	dxManager->BeginDraw();

	dxManager->Render(psoManager.get(), OffScreenManager::GetInstance()->GetFinalSrvIndex());

	///---------------------------------------------------------
	/// UI, ImGui描画（バックバッファ上で最終）
	///---------------------------------------------------------
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif // DEBUG

	///---------------------------------------------------------
	/// フレーム終了
	///---------------------------------------------------------
	dxManager->EndDraw();
}

void MyGameTitle::RemoveObjects()
{
	RendererManager::GetInstance()->RemoveDeadObjects();
	CollisionManager::GetInstance()->RemoveDeadObjects();
	Object3dManager::GetInstance()->RemoveDeadObject();
}

