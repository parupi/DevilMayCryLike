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
	auto commandList = dxManager_->GetCommandContext();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
		gBuffer_->GetRTVHandle(GBufferManager::GBufferType::Albedo),
		gBuffer_->GetRTVHandle(GBufferManager::GBufferType::Normal)
	};

	// Depth
	auto dsv = gBuffer_->GetDSVHandle();

	commandList->SetRenderTargets(rtvs, _countof(rtvs), &dsv);

	// クリア
	float clearColor[4] = { 0.6f, 0.5f, 0.1f, 1.0f };
	commandList->ClearRenderTarget(rtvs[0], clearColor);
	commandList->ClearRenderTarget(rtvs[1], clearColor);

	commandList->ClearDepth(dsv);

	// ViewPort / Scissor は Backbuffer と同じでよい
	commandList->SetViewportAndScissor(dxManager_->GetMainViewport(), dxManager_->GetMainScissorRect());
}

void GBufferPath::End()
{
}
