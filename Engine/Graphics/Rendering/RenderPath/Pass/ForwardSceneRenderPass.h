#pragma once
#include "Graphics/Rendering/RenderPath/IRenderPass.h"
#include "Graphics/Rendering/RenderPath/Impl/ForwardRenderPath.h"
#include "Core/EngineContext.h"
#include <memory>

class ForwardSceneRenderPass : public IRenderPass {
public:
	void Initialize(const EngineContext& ctx, const SharedRenderTarget& sharedRT);
	void Execute() override;
	const char* GetName() const override { return "ForwardScene"; }

private:
	EngineContext ctx_{};
	std::unique_ptr<ForwardRenderPath> forwardPath_;
	SharedRenderTarget sharedRT_{};
};
