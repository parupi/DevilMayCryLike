#include "FxaaEffect.h"
#include "OffScreenManager.h"

FxaaEffect::FxaaEffect(const std::string& name)
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	auto* resourceManager = dxManager_->GetResourceManager();
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(FxaaData), L"FxaaEffect");
	effectDataPtr_ = reinterpret_cast<FxaaData*>(resourceManager->Map(effectHandle_));
	*effectDataPtr_ = FxaaData{};
	SetActive(true);
}

FxaaEffect::~FxaaEffect()
{
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	effectHandle_ = 0;
}

void FxaaEffect::Update()
{
	// ping-pong の大きさ（= ポストエフェクトのビューポート）から1ピクセルの幅を出す。
	// Debug の Game RT と Release のバックバッファで大きさが違っても、ping-pong は常にこの大きさ
	const D3D12_VIEWPORT& vp = OffScreenManager::GetInstance().GetViewport();
	effectDataPtr_->rcpFrameX = vp.Width > 0.0f ? 1.0f / vp.Width : 0.0f;
	effectDataPtr_->rcpFrameY = vp.Height > 0.0f ? 1.0f / vp.Height : 0.0f;
}

void FxaaEffect::Draw()
{
	auto* cmd = dxManager_->GetCommandList();
	cmd->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kFxaa));
	cmd->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	cmd->SetGraphicsRootDescriptorTable(0, inputSrv_);
	cmd->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));
	cmd->DrawInstanced(3, 1, 0, 0);
}
