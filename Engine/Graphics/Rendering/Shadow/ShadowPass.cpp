#include "ShadowPass.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/Shadow/CascadedShadowMap.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
#include "Graphics/Resource/GpuResourceFactory.h"
#include <World3D/Object/Object3dManager.h>

void ShadowPass::Initialize(DirectXManager* dxManager, PSOManager* psoManager,
	CascadedShadowMap* shadowMap, Object3dManager* object3dManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
	shadowMap_ = shadowMap;
	object3dManager_ = object3dManager;
}

void ShadowPass::BeginDraw()
{
	auto* commandList = dxManager_->GetCommandContext()->GetCommandList();

	commandList->SetPipelineState(psoManager_->GetCSMPSO());
	commandList->SetGraphicsRootSignature(psoManager_->GetCSMRootSignature());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ShadowPass::Execute()
{
	for (uint32_t i = 0; i < kCascadeCount; ++i) {
		shadowMap_->BeginCascade(i);

		shadowMap_->Bind(1, i);

		object3dManager_->DrawShadow();

		shadowMap_->EndCascade(i);
	}
}