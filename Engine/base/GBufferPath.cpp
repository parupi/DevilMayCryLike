#include "GBufferPath.h"
#include "DirectXManager.h"
#include "GBufferManager.h"

void GBufferPath::Initialize(DirectXManager* dxManager, GBufferManager* gBuffer)
{
	dxManager_ = dxManager;
	gBuffer_ = gBuffer;
}

void GBufferPath::Begin()
{
	auto commandContext = dxManager_->GetCommandContext();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
		gBuffer_->GetRTVHandle(GBufferManager::GBufferType::Albedo),
		gBuffer_->GetRTVHandle(GBufferManager::GBufferType::Normal)
	};

	// Depth
	auto dsv = gBuffer_->GetDSVHandle();

	commandContext->SetRenderTargets(rtvs, _countof(rtvs), &dsv);

	// クリア
	float clearColor[4] = { 0.6f, 0.5f, 0.1f, 1.0f };
	commandContext->ClearRenderTarget(rtvs[0], clearColor);
	commandContext->ClearRenderTarget(rtvs[1], clearColor);

	commandContext->ClearDepth(dsv);

	// ViewPort / Scissor は BackBuffer と同じでよい
	commandContext->SetViewportAndScissor(dxManager_->GetMainViewport(), dxManager_->GetMainScissorRect());
}

void GBufferPath::End()
{
	// GeometryPassでGBufferに書き込み終わったので、SRVへ遷移
	gBuffer_->TransitionAllToReadable();
}
