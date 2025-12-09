#include "ForwardRenderPath.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Resource/GpuResourceFactory.h"

void ForwardRenderPath::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;

	CreateResource();
}

void ForwardRenderPath::BeginDraw()
{
	auto cmd = dxManager_->GetCommandList();
	auto* commandContext = dxManager_->GetCommandContext();

	// 出力リソースを RT に戻す
	commandContext->TransitionResource(
		rtvResource_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	dxManager_->GetCommandContext()->TransitionResource(
		depthBuffer_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);

	// パイプラインのセット
	cmd->SetPipelineState(psoManager_->GetLightingPathPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetLightingPathSignature());

	// 描画ターゲットの設定
	commandContext->SetRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(rtvIndex_), dxManager_->GetDsvManager()->GetDsvHandle(dsvIndex_));

	// depthのクリア
	commandContext->ClearDepth(dxManager_->GetDsvManager()->GetDsvHandle(dsvIndex_));
	// rtvのクリア
	float clearColor[4] = { 0.6f, 0.5f, 0.1f, 1.0f };
	commandContext->ClearRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(rtvIndex_), clearColor);

	ID3D12DescriptorHeap* heaps[] = { dxManager_->GetSrvManager()->GetHeap() };
	dxManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);
}

void ForwardRenderPath::EndDraw()
{
	auto* commandContext = dxManager_->GetCommandContext();

	// 読み込み用に変更
	commandContext->TransitionResource(
		rtvResource_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	dxManager_->GetCommandContext()->TransitionResource(
		depthBuffer_.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
}

void ForwardRenderPath::CreateResource()
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
	

	dxManager_->GetCommandContext()->TransitionResource(
		rtvResource_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

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

	dxManager_->GetCommandContext()->TransitionResource(
		depthBuffer_.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	// SRV の生成
	srvForDepthIndex_ = dxManager_->GetSrvManager()->Allocate();
	dxManager_->GetSrvManager()->CreateSRVforTexture2D(srvForDepthIndex_, depthBuffer_.Get(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1);
}
