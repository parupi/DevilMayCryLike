#include "RenderPipeline.h"
#include "Pass/SkinningRenderPass.h"
#include "Pass/ShadowRenderPass.h"
#include "Pass/GBufferRenderPass.h"
#include "Pass/LightingRenderPass.h"
#include "Pass/ForwardSceneRenderPass.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Rendering/PostEffect/OffScreenManager.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Text/FontManager.h"
#include "Scene/Transition/TransitionManager.h"
#include "Utility/ScopeProfiler.h"
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

	// --- GBufferManager ---
	gBufferManager_ = std::make_unique<GBufferManager>();
	gBufferManager_->Initialize(ctx_.dxManager);

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
	gBufferManager_->Finalize();
	rtvResource_.Reset();
	depthBuffer_.Reset();
}

void RenderPipeline::Execute() {
	// このフレームで新しく出てきた文字をアトラスへ上げる。
	// アトラスを読む描画より前で、かつフレームに1回で済むのがここ
	FontManager::GetInstance().FlushAtlases();

	// 各描画パスを順番に実行
	static const char* kPassNames[] = { "Pass:Skinning", "Pass:Shadow", "Pass:GBuffer", "Pass:Lighting", "Pass:Forward" };
	for (size_t i = 0; i < passes_.size(); ++i) {
		PROF_SCOPE(i < std::size(kPassNames) ? kPassNames[i] : "Pass:?");
		passes_[i]->Execute();
	}

	// OffScreen / PostEffect / 最終合成
	{
		PROF_SCOPE("PostEffect");
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

	{
		// Present とGPU待ち。ここが大きければGPUバウンド
		PROF_SCOPE("EndDraw(GPU待ち)");
		ctx_.dxManager->EndDraw();
	}
}
