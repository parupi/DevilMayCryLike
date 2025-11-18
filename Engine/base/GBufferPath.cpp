#include "GBufferPath.h"
#include "DirectXManager.h"
#include "GBufferManager.h"
#include "base/PSOManager.h"
#include <scene/SceneManager.h>

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
		gBuffer_->GetRTVHandle(GBufferManager::GBufferType::Normal)
	};

	// Depth
	auto dsv = gBuffer_->GetDSVHandle();

	commandContext->SetRenderTargets(rtvs, _countof(rtvs), &dsv);

	// クリア
	float clearColor[4] = { 0.6f, 0.5f, 0.1f, 1.0f };
	clearColor[0] = powf(0.6f, 2.2f);
	clearColor[1] = powf(0.5f, 2.2f);
	clearColor[2] = powf(0.1f, 2.2f);
	commandContext->ClearRenderTarget(rtvs[0], clearColor);
	commandContext->ClearRenderTarget(rtvs[1], clearColor);

	commandContext->ClearDepth(dsv);
}

void GBufferPath::Draw()
{
	auto* cmd = dxManager_->GetCommandList();

	cmd->SetPipelineState(psoManager_->GetDeferredPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetDeferredSignature());

	// SRVをセット
	cmd->SetGraphicsRootDescriptorTable(2, inputSrv_);

	// GBufferに対する実際の描画処理
	SceneManager::GetInstance()->Draw();
}

void GBufferPath::End()
{
	// GeometryPassでGBufferに書き込み終わったので、SRVへ遷移
	gBuffer_->TransitionAllToReadable();
}
