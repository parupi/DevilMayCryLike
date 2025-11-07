#include "GBufferPass.h"

void GBufferPass::Initialize(DirectXManager* dxManager)
{
    dxManager_ = dxManager;
   // dxManager_->CreateGBuffer(WindowManager::kClientWidth, WindowManager::kClientHeight);

    const GBuffer& gBuffer = dxManager_->GetGBuffer();
    rtvHandles_[0] = gBuffer.rtvHandles[0]; // albedo
    rtvHandles_[1] = gBuffer.rtvHandles[1]; // normal
    dsvHandle_ = gBuffer.dsvHandle;
}

void GBufferPass::Begin()
{
    auto cmdList = dxManager_->GetCommandList();
    cmdList->OMSetRenderTargets(2, rtvHandles_, FALSE, &dsvHandle_);

    float clearColor[4] = { 0.6f, 0.5f, 0.1f, 1.0f };
    cmdList->ClearRenderTargetView(rtvHandles_[0], clearColor, 0, nullptr);
    cmdList->ClearRenderTargetView(rtvHandles_[1], clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // IA設定
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void GBufferPass::End()
{
}
