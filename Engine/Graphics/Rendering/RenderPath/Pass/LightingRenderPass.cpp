#include "LightingRenderPass.h"
#include "World3D/Light/LightManager.h"
#include "World3D/Camera/CameraManager.h"

void LightingRenderPass::Initialize(const EngineContext& ctx, GBufferManager* gBufferManager,
	const SharedRenderTarget& sharedRT)
{
	ctx_ = ctx;
	sharedRT_ = sharedRT;

	lightingPath_ = std::make_unique<LightingPath>();
	lightingPath_->Initialize(ctx_.dxManager, gBufferManager, ctx_.psoManager);
}

void LightingRenderPass::Execute()
{
	auto* commandCtx = ctx_.dxManager->GetCommandContext();

	commandCtx->TransitionResource(sharedRT_.rtvResource,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandCtx->TransitionResource(sharedRT_.depthBuffer,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	lightingPath_->Begin(sharedRT_.rtvIndex, 0);

	ctx_.lightManager->GetCSM()->BindSrv();
	ctx_.lightManager->GetCSM()->BindCascadeCB(5);
	ctx_.lightManager->BindLightsToShader();
	ctx_.cameraManager->BindCameraToShader();

	lightingPath_->End();

	commandCtx->TransitionResource(sharedRT_.rtvResource,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandCtx->TransitionResource(sharedRT_.depthBuffer,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
