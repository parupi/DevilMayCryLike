#include "OffScreenManager.h"
#include "Graphics/Resource/SrvManager.h"
#include <math/function.h>

OffScreenManager* OffScreenManager::instance = nullptr;
std::once_flag OffScreenManager::initInstanceFlag;

OffScreenManager* OffScreenManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new OffScreenManager();
		});
	return instance;
}

void OffScreenManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	assert(dxManager);
	dxManager_ = dxManager;
	psoManager_ = psoManager;

	viewport_ = { 0.0f, 0.0f, WindowManager::kClientWidth, WindowManager::kClientHeight, 0.0f, 1.0f };
	scissorRect_ = { 0, 0, WindowManager::kClientWidth, WindowManager::kClientHeight };

	clearValue_.Color[0] = 0.0f;
	clearValue_.Color[1] = 0.0f;
	clearValue_.Color[2] = 0.0f;
	clearValue_.Color[3] = 1.0f;

	// ping/pong を作る（リソース作成 → RTV/SRV を Manager を通して作成）
	for (int i = 0; i < 2; ++i) {
		pingPongBuffers_[i] = CreateOffScreenRenderTarget();
		// create RTV index via RtvManager and create SRV index via SrvManager
		rtvIndices_[i] = CreateRTVForResource(pingPongBuffers_[i]);
		srvIndices_[i] = CreateSRVForResource(pingPongBuffers_[i]);

		// store handles for convenience
		rtvHandles_[i] = dxManager_->GetRtvManager()->GetCPUDescriptorHandle(rtvIndices_[i]);
		srvHandles_[i] = dxManager_->GetSrvManager()->GetGPUDescriptorHandle(srvIndices_[i]);

		dxManager_->GetCommandContext()->TransitionResource(
			pingPongBuffers_[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
	}

	ping_ = 0;
	pong_ = 1;
}

void OffScreenManager::Finalize()
{
	// オフスクリーンエフェクトの解放
	effects_.clear();

	for (auto& buffer : pingPongBuffers_) {
		buffer.Reset();
	}

	dxManager_ = nullptr;
	psoManager_ = nullptr;

	delete instance;
	instance = nullptr;
}

void OffScreenManager::Update()
{
	for (size_t i = 0; i < effects_.size(); i++) {
		effects_[i]->Update();
	}
}

void OffScreenManager::ExecutePostEffects()
{
	D3D12_GPU_DESCRIPTOR_HANDLE inputSrv = srvHandles_[pong_];
	// 1つでも有効なpathが動いたか判定
	bool anyExecuted = false;

	for (auto& path : paths_) {
		if (!path->IsActive()) continue;
		// 実行された
		anyExecuted = true;
		path->SetInputSRV(inputSrv);
		path->SetOutput(pingPongBuffers_[ping_].Get(), rtvHandles_[ping_]);
		path->SetViewport(viewport_, scissorRect_);

		path->Execute();

		std::swap(ping_, pong_);
		inputSrv = srvHandles_[pong_];

		finalPostEffectSrv_ = path->GetOutputSRV();
		outputSrvIndex_ = path->GetOutputSRVIndex();
	}
	// pathが一つも無かった場合のフォールバック
	if (!anyExecuted) {
		// 最初の入力を最終出力にする
		finalPostEffectSrv_ = srvHandles_[pong_];
		outputSrvIndex_ = srvIndices_[pong_];
	}
}

void OffScreenManager::AddEffect(std::unique_ptr<BaseOffScreen> effect)
{
	PostEffectPath* path = new PostEffectPath(effect.get());
	paths_.emplace_back(path);
	effects_.push_back(std::move(effect));
}

BaseOffScreen* OffScreenManager::FindEffect(const std::string& name)
{
	for (auto& effect : effects_) {
		if (!effect) continue;

		if (effect->GetName() == name) {
			return effect.get();
		}
	}
	return nullptr;
}

std::vector<BaseOffScreen*> OffScreenManager::GetEffects()
{
	std::vector<BaseOffScreen*> effects;
	for (auto& effect : effects_) {
		effects.push_back(effect.get());
	}
	return effects;
}

void OffScreenManager::BeginDrawToPingPong()
{
	// public API kept for external use if needed:
	dxManager_->GetCommandContext()->TransitionResource(
		pingPongBuffers_[ping_].Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// set RTV & DSV
	dxManager_->GetCommandContext()->SetRenderTarget(rtvHandles_[ping_], dxManager_->GetDsvManager()->GetDsvHandle());
	//dxManager_->GetCommandContext()->ClearRenderTarget(rtvHandles_[ping_], clearValue_.Color);
	dxManager_->GetCommandContext()->SetViewportAndScissor(viewport_, scissorRect_);
}

void OffScreenManager::EndDrawToPingPong()
{
	dxManager_->GetCommandContext()->TransitionResource(
		pingPongBuffers_[ping_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);

	std::swap(ping_, pong_);
}

void OffScreenManager::CopyLightingToPing(uint32_t lightingSrv)
{
	// 描画後にリソース状態を GENERIC_READ に戻す（ping_ を使う）
	dxManager_->GetCommandContext()->TransitionResource(
		pingPongBuffers_[ping_].Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	auto* cmd = dxManager_->GetCommandList();

	// DescriptorHeap を先にバインド
	ID3D12DescriptorHeap* heaps[] = { dxManager_->GetSrvManager()->GetHeap() };
	cmd->SetDescriptorHeaps(1, heaps);

	// --- RootSignature と PSO を正しくセット ---
	// PSO に紐づく RootSignature はグラフィックス用なので SetGraphicsRootSignature を使う
	cmd->SetPipelineState(psoManager_->GetFinalCompositePSO(true));
	cmd->SetGraphicsRootSignature(psoManager_->GetFinalCompositeRootSignature());

	// ビューポート/シザーはコマンドコンテキスト経由でもOK（同一CLか確認）
	dxManager_->GetCommandContext()->SetViewportAndScissor(viewport_, scissorRect_);

	// 出力先は ping の RTV（ping_ を使う）
	dxManager_->GetCommandContext()->SetRenderTarget(rtvHandles_[ping_], dxManager_->GetDsvManager()->GetDsvHandle());
	dxManager_->GetCommandContext()->ClearRenderTarget(rtvHandles_[ping_], clearValue_.Color);

	// プリミティブ設定
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// t0 に LightingTexture をバインド
	cmd->SetGraphicsRootDescriptorTable(0, dxManager_->GetSrvManager()->GetGPUDescriptorHandle(lightingSrv));

	// フルスクリーントライアングルを描画
	cmd->DrawInstanced(3, 1, 0, 0);

	// 描画後にリソース状態を GENERIC_READ に戻す（ping_ を使う）
	dxManager_->GetCommandContext()->TransitionResource(
		pingPongBuffers_[ping_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);
}


Microsoft::WRL::ComPtr<ID3D12Resource> OffScreenManager::CreateOffScreenRenderTarget()
{
	GpuResourceFactory::TextureDesc desc;
	desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.usage = GpuResourceFactory::Usage::RenderTarget;
	desc.clearColor[0] = 0.0f;
	desc.clearColor[1] = 0.0f;
	desc.clearColor[2] = 0.0f;
	desc.clearColor[3] = 1.0f;
	return dxManager_->GetResourceFactory()->CreateTexture2D(desc);
}

uint32_t OffScreenManager::CreateRTVForResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
	// allocate index from RtvManager
	uint32_t index = dxManager_->GetRtvManager()->Allocate();
	// create RTV at manager
	dxManager_->GetRtvManager()->CreateRTV(index, resource.Get());
	return index;
}

uint32_t OffScreenManager::CreateSRVForResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
	uint32_t index = dxManager_->GetSrvManager()->Allocate();
	dxManager_->GetSrvManager()->CreateSRVforTexture2D(index, resource.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, 1);
	return index;
}
