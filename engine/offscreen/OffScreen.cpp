#include "OffScreen.h"
#ifdef _DEBUG
#include <imgui.h>
#endif

void OffScreen::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;

}

void OffScreen::Finalize()
{
}

void OffScreen::Update()
{
	ImGui::Begin("OffScreen");
	// エフェクト名の文字列配列
	const char* effectNames[] = {
		"None",
		"Gray",
		"Vignette",
		"Smooth",
		"Gauss",
		"OutLine",
		"Depth"
	};

	// 現在のエフェクト（整数にキャスト）
	int currentEffect = static_cast<int>(effectType_);

	// ImGuiのコンボボックス
	if (ImGui::Combo("Effect Type", &currentEffect, effectNames, IM_ARRAYSIZE(effectNames))) {
		// 選択が変わったときに enum に変換
		effectType_ = static_cast<OffScreenEffectType>(currentEffect);
	}
	ImGui::End();
}

void OffScreen::Draw(/*OffScreenEffectType effectType*/)
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(effectType_).Get());
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature().Get());
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxManager_->GetSrvHandle().second);
	dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 1);
}