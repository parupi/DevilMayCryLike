#include "ShadowRenderPass.h"
#include <World3D/Light/LightManager.h>

void ShadowRenderPass::Initialize(const EngineContext& ctx) {
	shadowPass_ = std::make_unique<ShadowPass>();
	shadowPass_->Initialize(ctx.dxManager, ctx.psoManager, ctx.lightManager->GetCSM(), ctx.object3dManager);
}

void ShadowRenderPass::Execute() {
	shadowPass_->BeginDraw();
	shadowPass_->Execute();
}
