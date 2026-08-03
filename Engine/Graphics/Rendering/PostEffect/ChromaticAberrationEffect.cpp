#include "ChromaticAberrationEffect.h"
#include "OffScreenManager.h"

ChromaticAberrationEffect::ChromaticAberrationEffect(const std::string& name)
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	CreateEffectResource();
}

ChromaticAberrationEffect::~ChromaticAberrationEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectHandle_ = 0;
}

void ChromaticAberrationEffect::Update()
{
	// CPU → GPU に全フィールドをコピー（パディング含む）
	*effectDataPtr_ = effectData_;
}

void ChromaticAberrationEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kChromaticAberration));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void ChromaticAberrationEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(ChromaticAberrationData), L"ChromaticAberrationEffect");
	effectDataPtr_ = reinterpret_cast<ChromaticAberrationData*>(resourceManager->Map(effectHandle_));
	*effectDataPtr_ = effectData_;
}
