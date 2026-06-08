#pragma once
#include <d3d12.h>
#include <cstdint>

// 複数パスで共有するRTVとDSVリソース (RenderPipelineが所有、各パスは参照のみ)
struct SharedRenderTarget {
	ID3D12Resource* rtvResource = nullptr;
	uint32_t rtvIndex = 0;
	ID3D12Resource* depthBuffer = nullptr;
	uint32_t dsvIndex = 0;
};

class IRenderPass {
public:
	virtual ~IRenderPass() = default;
	virtual void Execute() = 0;
	virtual const char* GetName() const = 0;
};
