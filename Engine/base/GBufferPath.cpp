#include "GBufferPath.h"
#include "DirectXManager.h"
#include "GBufferManager.h"
#include "base/PSOManager.h"
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

    commandContext->SetRenderTargets(rtvs, _countof(rtvs), &dsv);

    // --- 各リソースのクリア値を取得 ---
    D3D12_CLEAR_VALUE albedoClear{};
    albedoClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    albedoClear.Color[0] = 0.0f;
    albedoClear.Color[1] = 0.0f;
    albedoClear.Color[2] = 0.0f;
    albedoClear.Color[3] = 1.0f;

    D3D12_CLEAR_VALUE normalClear{};
    normalClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    normalClear.Color[0] = 0.5f;
    normalClear.Color[1] = 0.5f;
    normalClear.Color[2] = 1.0f;
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
