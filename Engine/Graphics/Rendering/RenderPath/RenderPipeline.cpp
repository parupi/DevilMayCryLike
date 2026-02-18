#include "RenderPipeline.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include <3d/Light/LightManager.h>
#include <3d/Object/Object3dManager.h>
#include <3d/Camera/CameraManager.h>
#include <3d/SkySystem/SkySystem.h>
#include <scene/Transition/TransitionManager.h>
#include <scene/SceneManager.h>
#include <debuger/ImGuiManager.h>
#include <offscreen/OffScreenManager.h>
#include <base/Particle/ParticleManager.h>

void RenderPipeline::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;

	gBufferManager = std::make_unique<GBufferManager>();
	gBufferManager->Initialize(dxManager_);

	gBufferPath = std::make_unique<GBufferPath>();
	gBufferPath->Initialize(dxManager_, gBufferManager.get(), psoManager);

	lightingPath = std::make_unique<LightingPath>();
	lightingPath->Initialize(dxManager_, gBufferManager.get(), psoManager);

	forwardPath = std::make_unique<ForwardRenderPath>();
	forwardPath->Initialize(dxManager_, psoManager);

	compositePath = std::make_unique<CompositePath>();
	compositePath->Initialize(dxManager_, psoManager);
	// rtvResourceの生成
	auto* rtvManager = dxManager_->GetRtvManager();
	auto* srvManager = dxManager_->GetSrvManager();

	// リソースの生成
	GpuResourceFactory::TextureDesc desc;
	desc.clearColor[0] = 0.6f;
	desc.clearColor[1] = 0.5f;
	desc.clearColor[2] = 0.1f;
	desc.clearColor[3] = 1.0f;
	desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.usage = GpuResourceFactory::Usage::RenderTarget;
	rtvResource_ = dxManager_->GetResourceFactory()->CreateTexture2D(desc);

	// rtvの生成
	rtvIndex_ = dxManager_->GetRtvManager()->Allocate();
	dxManager_->GetRtvManager()->CreateRTV(rtvIndex_, rtvResource_.Get());

	// SRV の生成
	srvIndex_ = dxManager_->GetSrvManager()->Allocate();
	dxManager_->GetSrvManager()->CreateSRVforTexture2D(srvIndex_, rtvResource_.Get(), desc.format, 1);

	// DSV生成
	GpuResourceFactory::TextureDesc dsvDesc{};
	dsvDesc.format = DXGI_FORMAT_R24G8_TYPELESS;
	dsvDesc.usage = GpuResourceFactory::Usage::DepthStencil;

	depthBuffer_ = dxManager_->GetResourceFactory()->CreateTexture2D(dsvDesc);
	depthBuffer_->SetName(L"DepthBufferForForward");

	dsvIndex_ = dxManager_->GetDsvManager()->Allocate();
	dxManager_->GetDsvManager()->CreateDsv(dsvIndex_, depthBuffer_.Get(), DXGI_FORMAT_D24_UNORM_S8_UINT);

	// SRV の生成
	//srvForDepthIndex_ = dxManager_->GetSrvManager()->Allocate();
	//dxManager_->GetSrvManager()->CreateSRVforTexture2D(srvForDepthIndex_, depthBuffer_.Get(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1);

	TransitionToSRV();
}

void RenderPipeline::Finalize()
{
	gBufferManager->Finalize();
	gBufferPath.reset();
	lightingPath.reset();
}

void RenderPipeline::Execute(PSOManager* psoManager)
{
	///---------------------------------------------------------
	/// GBufferPath（Deferredの各バッファ生成）
	///---------------------------------------------------------
	
	TransitionToRTV();
	
	gBufferPath->Begin(dsvIndex_);

	Object3dManager::GetInstance()->DrawDeferred();

	gBufferPath->End();

	TransitionToSRV();

	///---------------------------------------------------------
	/// LightingPath（GBuffer結果を使って描画）
	///---------------------------------------------------------
	TransitionToRTV();

	lightingPath->Begin(rtvIndex_);

	LightManager::GetInstance()->BindLightsToShader();
	CameraManager::GetInstance()->BindCameraToShader();

	lightingPath->End();

	TransitionToSRV();

	///---------------------------------------------------------
	/// ForwardRenderPath
	///---------------------------------------------------------

	TransitionToRTV();
	// 描画前処理
	forwardPath->BeginDraw(rtvIndex_, dsvIndex_);
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

	TransitionToSRV();

	///---------------------------------------------------------
	/// OffScreen（ポストエフェクト前の下準備 or 前景エフェクト）
	///---------------------------------------------------------
	OffScreenManager::GetInstance()->CopyLightingToPing(srvIndex_);

	OffScreenManager::GetInstance()->BeginDrawToPingPong();

	OffScreenManager::GetInstance()->EndDrawToPingPong();

	///---------------------------------------------------------
	/// PostEffectPath（Ping-Pong結果から最終1枚に統合）
	///---------------------------------------------------------
	OffScreenManager::GetInstance()->ExecutePostEffects();

	dxManager_->BeginDraw();

	dxManager_->Render(psoManager, OffScreenManager::GetInstance()->GetFinalSrvIndex());

	///---------------------------------------------------------
	/// UI, ImGui描画（バックバッファ上で最終）
	///---------------------------------------------------------
#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif // DEBUG

	///---------------------------------------------------------
	/// フレーム終了
	///---------------------------------------------------------
	dxManager_->EndDraw();
}

void RenderPipeline::TransitionToRTV()
{
	auto* commandContext = dxManager_->GetCommandContext();

	commandContext->TransitionResource(
		rtvResource_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	commandContext->TransitionResource(
		depthBuffer_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
}

void RenderPipeline::TransitionToSRV()
{
	auto* commandContext = dxManager_->GetCommandContext();

	commandContext->TransitionResource(
		rtvResource_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	commandContext->TransitionResource(
		depthBuffer_.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
}
