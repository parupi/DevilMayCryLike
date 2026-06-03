#include "VignetteEffect.h"
#include "OffScreenManager.h"
#include <imgui/imgui.h>

VignetteEffect::VignetteEffect(const std::string& name)
{
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance()->GetDXManager();
	psoManager_ = OffScreenManager::GetInstance()->GetPSOManager();

	CreateEffectResource();
}

VignetteEffect::~VignetteEffect()
{
	// 借りてるポインタを破棄
	dxManager_ = nullptr;
	psoManager_ = nullptr;
	// 生成したリソースの削除
	effectHandle_ = 0;
}

void VignetteEffect::Update()
{
#ifdef _DEBUG
	ImGui::Begin(name_.c_str());
	ImGui::Checkbox("isActive", &isActive_);
	ImGui::DragFloat("radius",    &effectData_.radius,    0.01f);
	ImGui::DragFloat("intensity", &effectData_.intensity, 0.01f);
	ImGui::DragFloat("softness",  &effectData_.softness,  0.01f);
	float col[3] = { effectData_.colorR, effectData_.colorG, effectData_.colorB };
	if (ImGui::ColorEdit3("edgeColor", col)) {
		effectData_.colorR = col[0];
		effectData_.colorG = col[1];
		effectData_.colorB = col[2];
	}
	ImGui::End();
#endif // _DEBUG

	// CPU → GPU に全フィールドをコピー（パディング含む）
	*effectDataPtr_ = effectData_;
}

void VignetteEffect::Draw()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kVignette));
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, inputSrv_);

	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(effectHandle_));

	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void VignetteEffect::CreateEffectResource()
{
	auto* resourceManager = dxManager_->GetResourceManager();
	// ヴィネット用のリソースを作る
	effectHandle_ = resourceManager->CreateUploadBuffer(sizeof(VignetteEffectData), L"GaussianEffect");
	// 書き込むためのアドレスを取得
	effectDataPtr_ = reinterpret_cast<VignetteEffectData*>(resourceManager->Map(effectHandle_));
	// 初期値を設定
	effectDataPtr_->radius = 0.5f;
	effectDataPtr_->intensity = 0.5f;
	effectDataPtr_->softness = -0.1f;
}
