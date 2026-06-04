#pragma once
#include "Graphics/Rendering/RenderPath/IRenderPass.h"
#include "Graphics/Rendering/RenderPath/Impl/GBufferPath.h"
#include "Graphics/Rendering/RenderPath/Impl/GBufferManager.h"
#include "Core/EngineContext.h"
#include <memory>

class GBufferRenderPass : public IRenderPass {
public:
	void Initialize(const EngineContext& ctx, GBufferManager* gBufferManager,
	                const SharedRenderTarget& sharedRT);
	void Execute() override;
	const char* GetName() const override { return "GBuffer"; }

private:
	EngineContext ctx_{};
	std::unique_ptr<GBufferPath> gBufferPath_;
	SharedRenderTarget sharedRT_{};
};
