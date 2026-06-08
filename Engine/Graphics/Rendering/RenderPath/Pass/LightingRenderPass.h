#pragma once
#include "Graphics/Rendering/RenderPath/IRenderPass.h"
#include "Graphics/Rendering/RenderPath/Impl/LightingPath.h"
#include "Graphics/Rendering/RenderPath/Impl/GBufferManager.h"
#include "Core/EngineContext.h"
#include <memory>

class LightingRenderPass : public IRenderPass {
public:
	void Initialize(const EngineContext& ctx, GBufferManager* gBufferManager,
	                const SharedRenderTarget& sharedRT);
	void Execute() override;
	const char* GetName() const override { return "Lighting"; }

private:
	EngineContext ctx_{};
	std::unique_ptr<LightingPath> lightingPath_;
	SharedRenderTarget sharedRT_{};
};
