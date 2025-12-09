#include "GBufferPath.h"
#include "Graphics/Device/DirectXManager.h"
#include "GBufferManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include <scene/SceneManager.h>
#include <math/function.h>

GBufferPath::~GBufferPath()
{
	dxManager_ = nullptr;
	gBuffer_ = nullptr;
	psoManager_ = nullptr;
}

void GBufferPath::Initialize(DirectXManager* dxManager, GBufferManager* gBuffer, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	gBuffer_ = gBuffer;
	psoManager_ = psoManager;
}

void GBufferPath::Begin()
{
	gBuffer_->TransitionAllToRT();

	auto commandContext = dxManager_->GetCommandContext();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
		gBuffer_->GetRTVHandle(GBufferManager::GBufferType::Albedo),
		gBuffer_->GetRTVHandle(GBufferManager::GBufferType::Normal),
		gBuffer_->GetRTVHandle(GBufferManager::GBufferType::WorldPos)
	};

	// Depth
	auto dsv = gBuffer_->GetDSVHandle();

	D3D12_VIEWPORT viewport{ 0.0f, 0.0f, WindowManager::kClientWidth, WindowManager::kClientHeight, 0.0f, 1.0f };
	D3D12_RECT scissorRect{ 0, 0, WindowManager::kClientWidth, WindowManager::kClientHeight };

	commandContext->SetViewportAndScissor(viewport, scissorRect);

	commandContext->SetRenderTargets(rtvs, _countof(rtvs), &dsv);

	// --- 各リソースのクリア値を取得 ---
	D3D12_CLEAR_VALUE albedoClear{};
	albedoClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	albedoClear.Color[0] = 0.6f;
	albedoClear.Color[1] = 0.5f;
	albedoClear.Color[2] = 0.1f;
	albedoClear.Color[3] = 1.0f;

	D3D12_CLEAR_VALUE normalClear{};
	normalClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	normalClear.Color[0] = 0.0f;
	normalClear.Color[1] = 0.5f;
	normalClear.Color[2] = 0.0f;
	normalClear.Color[3] = 1.0f;

	D3D12_CLEAR_VALUE worldPosClear{};
	worldPosClear.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	worldPosClear.Color[0] = 0.0f;
	worldPosClear.Color[1] = 0.0f;
	worldPosClear.Color[2] = 0.0f;
	worldPosClear.Color[3] = 1.0f;

	// --- クリア ---
	commandContext->ClearRenderTarget(rtvs[0], albedoClear.Color);
	commandContext->ClearRenderTarget(rtvs[1], normalClear.Color);
	commandContext->ClearRenderTarget(rtvs[2], worldPosClear.Color);

	commandContext->ClearDepth(dsv);

	auto* cmd = dxManager_->GetCommandList();

	cmd->SetPipelineState(psoManager_->GetDeferredPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetDeferredSignature());

	//srvHeapをセット
	ID3D12DescriptorHeap* heaps[] = { dxManager_->GetSrvManager()->GetHeap() };
	cmd->SetDescriptorHeaps(1, heaps);
}

void GBufferPath::End()
{
	// GeometryPassでGBufferに書き込み終わったので、SRVへ遷移
	gBuffer_->TransitionAllToReadable();
}
