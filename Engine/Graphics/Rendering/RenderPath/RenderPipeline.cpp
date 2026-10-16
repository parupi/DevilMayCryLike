#include "RenderPipeline.h"
#include "Pass/SkinningRenderPass.h"
#include "Pass/ShadowRenderPass.h"
#include "Pass/GBufferRenderPass.h"
#include "Pass/LightingRenderPass.h"
#include "Pass/SSAORenderPass.h"
#include "Pass/ForwardSceneRenderPass.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Rendering/PostEffect/OffScreenManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Graphics/Rendering/Environment/UpperFogRenderer.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Text/FontManager.h"
#include "Scene/Transition/TransitionManager.h"
#include "Utility/ScopeProfiler.h"
#include "Utility/GpuProfiler.h"
#include <iterator>
#ifdef _DEBUG
#include <Debugger/ImGuiManager.h>
#endif

void RenderPipeline::Initialize(const EngineContext& ctx) {
	ctx_ = ctx;

	// --- 共有リソースの生成 ---
	auto* rtvManager = ctx_.dxManager->GetRtvManager();
	auto* srvManager = ctx_.dxManager->GetSrvManager();

	GpuResourceFactory::TextureDesc rtvDesc;
	rtvDesc.clearColor[0] = 0.6f;
	rtvDesc.clearColor[1] = 0.5f;
	rtvDesc.clearColor[2] = 0.1f;
	rtvDesc.clearColor[3] = 1.0f;
	rtvDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.usage = GpuResourceFactory::Usage::RenderTarget;
	rtvResource_ = ctx_.dxManager->GetResourceFactory()->CreateTexture2D(rtvDesc);

	sharedRT_.rtvIndex = rtvManager->Allocate();
	rtvManager->CreateRTV(sharedRT_.rtvIndex, rtvResource_.Get());

	srvIndex_ = srvManager->Allocate();
	srvManager->CreateSRVforTexture2D(srvIndex_, rtvResource_.Get(), rtvDesc.format, 1);

	GpuResourceFactory::TextureDesc dsvDesc{};
	dsvDesc.format = DXGI_FORMAT_R24G8_TYPELESS;
	dsvDesc.usage = GpuResourceFactory::Usage::DepthStencil;
	depthBuffer_ = ctx_.dxManager->GetResourceFactory()->CreateTexture2D(dsvDesc);
	depthBuffer_->SetName(L"DepthBufferForForward");

	sharedRT_.dsvIndex = ctx_.dxManager->GetDsvManager()->Allocate();
	ctx_.dxManager->GetDsvManager()->CreateDsv(sharedRT_.dsvIndex, depthBuffer_.Get(), DXGI_FORMAT_D24_UNORM_S8_UINT);

	sharedRT_.rtvResource = rtvResource_.Get();
	sharedRT_.depthBuffer = depthBuffer_.Get();

	// 初期状態はSRV
	auto* commandCtx = ctx_.dxManager->GetCommandContext();
	commandCtx->TransitionResource(rtvResource_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandCtx->TransitionResource(depthBuffer_.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// 描画パスごとの GPU 処理時間
	GpuProfiler::GetInstance().Initialize(ctx_.dxManager);

	// --- GBufferManager ---
	gBufferManager_ = std::make_unique<GBufferManager>();
	gBufferManager_->Initialize(ctx_.dxManager);

	// ソフトパーティクル用。パーティクルは Forward パスで描かれ、その時点の GBuffer は読める状態にある
	if (ctx_.particleManager) {
		ctx_.particleManager->SetSceneWorldPositionSrv(
			gBufferManager_->GetSRVHandle(GBufferManager::GBufferType::WorldPos));
	}
	// 上部の霧の層も、壁や岩と交わる所を薄くするのに同じものを読む
	UpperFogRenderer::GetInstance().Initialize(ctx_.dxManager);
	UpperFogRenderer::GetInstance().SetSceneWorldPositionSrv(
		gBufferManager_->GetSRVHandle(GBufferManager::GBufferType::WorldPos));

	// --- パスを順番に登録 ---
	{
		// スキニングは影とGBufferの両方が結果を読むので必ず先頭
		auto pass = std::make_unique<SkinningRenderPass>();
		pass->Initialize(ctx_);
		passes_.push_back(std::move(pass));
	}
	{
		auto pass = std::make_unique<ShadowRenderPass>();
		pass->Initialize(ctx_);
		passes_.push_back(std::move(pass));
	}
	{
		auto pass = std::make_unique<GBufferRenderPass>();
		pass->Initialize(ctx_, gBufferManager_.get(), sharedRT_);
		passes_.push_back(std::move(pass));
	}
	{
		// SSAO は GBuffer の法線・ワールド座標を読み、結果をライティングへ渡す
		auto pass = std::make_unique<SSAORenderPass>();
		pass->Initialize(ctx_, gBufferManager_.get());
		passes_.push_back(std::move(pass));
	}
	{
		auto pass = std::make_unique<LightingRenderPass>();
		pass->Initialize(ctx_, gBufferManager_.get(), sharedRT_);
		passes_.push_back(std::move(pass));
	}
	{
		auto pass = std::make_unique<ForwardSceneRenderPass>();
		pass->Initialize(ctx_, sharedRT_);
		passes_.push_back(std::move(pass));
	}
}

void RenderPipeline::Finalize() {
	passes_.clear();
	GpuProfiler::GetInstance().Finalize();
	UpperFogRenderer::GetInstance().Finalize();
	gBufferManager_->Finalize();
	rtvResource_.Reset();
	depthBuffer_.Reset();
}

void RenderPipeline::Execute() {
	// このフレームで新しく出てきた文字をアトラスへ上げる。
	// アトラスを読む描画より前で、かつフレームに1回で済むのがここ
	FontManager::GetInstance().FlushAtlases();

	// 前のフレームの GPU 処理時間を読み、このフレームの計測を始める
	GpuProfiler::GetInstance().BeginFrame();

	// 各描画パスを順番に実行
	static const char* kPassNames[] = { "Pass:Skinning", "Pass:Shadow", "Pass:GBuffer", "Pass:SSAO", "Pass:Lighting", "Pass:Forward" };
	for (size_t i = 0; i < passes_.size(); ++i) {
		const char* name = i < std::size(kPassNames) ? kPassNames[i] : "Pass:?";
		PROF_SCOPE(name);
		GpuProfileScope gpuScope(name);
		passes_[i]->Execute();
	}

	// OffScreen / PostEffect / 最終合成
	{
		PROF_SCOPE("PostEffect");
		GpuProfileScope gpuScope("PostEffect");
		ctx_.offScreenManager->CopyLightingToPing(srvIndex_);
		ctx_.offScreenManager->BeginDrawToPingPong();
		ctx_.offScreenManager->EndDrawToPingPong();
		ctx_.offScreenManager->ExecutePostEffects();
	}

#ifdef _DEBUG
	// エディタ中はゲームの絵をバックバッファではなく専用のオフスクリーン(1280x720固定)へ描き、
	// ImGuiのGameウィンドウに ImGui::Image で表示する
	ctx_.imGuiManager->BeginGameViewRender();
#else
	ctx_.dxManager->BeginDraw();
#endif
	ctx_.dxManager->Render(ctx_.psoManager, ctx_.offScreenManager->GetFinalSrvIndex());

	{
		PROF_SCOPE("UI/Sprite");
		GpuProfileScope gpuScope("UI/Sprite");
		// UIはポストエフェクトの影響を受けないよう、合成後のバックバッファへ直接描く。
		// フェードはPersistentレイヤーのスプライトとして DrawUILayers() 内で描かれる。
		ctx_.spriteManager->DrawUILayers();
		// スプライトを使わないトランジション用のフック
		ctx_.transitionManager->Draw();
	}

#ifdef _DEBUG
	// ゲームの絵を出し終えたのでSRVへ戻し、バックバッファにはImGuiだけを描く
	ctx_.imGuiManager->EndGameViewRender();
	ctx_.dxManager->BeginDraw();
	{
		PROF_SCOPE("ImGui::Draw");
		ctx_.imGuiManager->Draw();
	}
#endif

	// このフレームの GPU 計測の結果を読み戻し用バッファへ（コマンドリストを閉じる前）
	GpuProfiler::GetInstance().EndFrame();

	{
		// Present とGPU待ち。ここが大きければGPUバウンド
		PROF_SCOPE("EndDraw(GPU待ち)");
		ctx_.dxManager->EndDraw();
	}
}
