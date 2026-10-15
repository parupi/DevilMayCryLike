#include "ToneMapEffect.h"
#include "OffScreenManager.h"
#include "Graphics/Rendering/Environment/RenderEnvironment.h"

ToneMapEffect::ToneMapEffect(const std::string& name)
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	auto* resourceManager = dxManager_->GetResourceManager();
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(ToneMapData), L"ToneMapEffect");
	effectDataPtr_ = reinterpret_cast<ToneMapData*>(resourceManager->Map(effectHandle_));
	*effectDataPtr_ = ToneMapData{};
	SetActive(false);
}

ToneMapEffect::~ToneMapEffect()
{
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	effectHandle_ = 0;
}

void ToneMapEffect::Update()
{
	// 使うかどうかはシーンの雰囲気が決める（洞窟なら有効）
	const RenderEnvironment::Params& env = RenderEnvironment::GetInstance().GetParams();
	SetActive(env.toneMapEnabled);
	effectDataPtr_->exposure = env.exposure;
	effectDataPtr_->shoulderStart = env.toneMapShoulderStart;
}

void ToneMapEffect::Draw()
{
	auto* cmd = dxManager_->GetCommandList();
	cmd->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kToneMap));
	cmd->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	cmd->SetGraphicsRootDescriptorTable(0, inputSrv_);
	cmd->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));
	cmd->DrawInstanced(3, 1, 0, 0);
}
