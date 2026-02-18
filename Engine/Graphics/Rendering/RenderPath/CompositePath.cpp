#include "CompositePath.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Resource/GpuResourceFactory.h"

void CompositePath::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;

	CreateResource();
}

void CompositePath::Composite(uint32_t forwardSrvIndex, uint32_t forwardDsvIndex, uint32_t DeferredSrvIndex, uint32_t deferredDsvIndex)
{
	auto* commandList = dxManager_->GetCommandList();
	auto* commandContext = dxManager_->GetCommandContext();

	// 出力リソースを RT に戻す
	commandContext->TransitionResource(
		rtvResource_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	ID3D12DescriptorHeap* heaps[] = { dxManager_->GetSrvManager()->GetHeap() };
	dxManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// パイプラインのセット
	commandList->SetPipelineState(psoManager_->GetCompositePSO());
	commandList->SetGraphicsRootSignature(psoManager_->GetCompositeRootSignature());

	// RTVをComposite用にセット
	commandContext->SetRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(rtvIndex_));

	// Deferred SRV → t0
	// Forward SRV  → t2
	commandList->SetGraphicsRootDescriptorTable(0, dxManager_->GetSrvManager()->GetGPUDescriptorHandle(DeferredSrvIndex));
	commandList->SetGraphicsRootDescriptorTable(1, dxManager_->GetSrvManager()->GetGPUDescriptorHandle(deferredDsvIndex));
	commandList->SetGraphicsRootDescriptorTable(2, dxManager_->GetSrvManager()->GetGPUDescriptorHandle(forwardSrvIndex));
	commandList->SetGraphicsRootDescriptorTable(3, dxManager_->GetSrvManager()->GetGPUDescriptorHandle(forwardDsvIndex));


	// FullScreenQuad描画
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);

	// 読み込み用に変更
	commandContext->TransitionResource(
		rtvResource_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
}

void CompositePath::CreateResource()
{
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

	// 描画開始時にSRVからRTVに変えるのでそれに合わせておく
	dxManager_->GetCommandContext()->TransitionResource(
		rtvResource_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	// SRV の生成（DirectXManager が最終合成で使えるようにする）
	srvIndex_ = dxManager_->GetSrvManager()->Allocate();
	dxManager_->GetSrvManager()->CreateSRVforTexture2D(srvIndex_, rtvResource_.Get(), desc.format, 1);
}
