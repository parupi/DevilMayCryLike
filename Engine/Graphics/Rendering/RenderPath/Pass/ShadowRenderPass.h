#pragma once
#include "Graphics/Rendering/RenderPath/IRenderPass.h"
#include "Graphics/Rendering/Shadow/ShadowPass.h"
#include "Core/EngineContext.h"
#include <memory>

class ShadowRenderPass : public IRenderPass {
public:
	void Initialize(const EngineContext& ctx);
	void Execute() override;
	const char* GetName() const override { return "Shadow"; }

private:
	std::unique_ptr<ShadowPass> shadowPass_;
};
