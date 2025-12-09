#include "PostEffectPath.h"
#include <base/DirectXManager.h>
#include <base/PSOManager.h>
#include <offscreen/OffScreenManager.h>
#include <offscreen/BaseOffScreen.h>

PostEffectPath::PostEffectPath(BaseOffScreen* effect)
{
    offscreen_ = OffScreenManager::GetInstance();
	dxManager_ = offscreen_->GetDXManager();
    psoManager_ = offscreen_->GetPSOManager();

    effect_ = effect;
}

void PostEffectPath::Execute()
{
    if (!effect_ || !effect_->IsActive()) return;

    auto* context = dxManager_->GetCommandContext();

    // 書き込み可能状態に
    context->TransitionResource(
        outputResource_,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_RENDER_TARGET
   );

    ID3D12DescriptorHeap* heaps[] = { dxManager_->GetSrvManager()->GetHeap() };
    dxManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

    // RTVセット
    context->SetRenderTarget(rtvHandle_);
    //context->ClearRenderTarget(rtvHandle_, offscreen_->GetClearColor());
    context->SetViewportAndScissor(viewport_, scissorRect_);

    // 実際の描画
    effect_->Draw();

    context->TransitionResource(
        outputResource_,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );

    // 最終的なSRVハンドルを保持
    auto* srvManager = dxManager_->GetSrvManager();
    // GPUハンドルを保持しておく（後でLightingPath等が参照）
    outputSrv_ = srvManager->GetGPUDescriptorHandle(outputSrvIndex_);
}

bool PostEffectPath::IsActive() const
{
    return effect_ && effect_->IsActive();
}

void PostEffectPath::SetInputSRV(D3D12_GPU_DESCRIPTOR_HANDLE srv)
{
    inputSrv_ = srv;
    if (effect_) {
        effect_->SetInputTexture(srv);
    }
}

void PostEffectPath::SetOutput(ID3D12Resource* target, D3D12_CPU_DESCRIPTOR_HANDLE rtv)
{
    outputResource_ = target;
    rtvHandle_ = rtv;

    if (!outputInitialized_) {
        auto* srvManager = dxManager_->GetSrvManager();
        outputSrvIndex_ = srvManager->CreateSRVFromResource(outputResource_);
        outputInitialized_ = true;
    }
}

void PostEffectPath::SetViewport(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
    viewport_ = vp;
    scissorRect_ = rect;
}
