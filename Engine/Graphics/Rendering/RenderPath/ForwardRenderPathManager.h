#pragma once
#include <array>
#include <memory>
#include "ForwardRenderPath.h"
#include "ForwardRenderTargetArray.h"

class DirectXManager;
class PSOManager;

class ForwardRenderPathManager
{
public:
	// 初期化
	void Initialize(DirectXManager* dxManager, PSOManager* psoManager);
	// レンダーパスの取得
	ForwardRenderPath* GetRenderPath(BlendCategory mode){ return paths_[(int)mode].get(); }

    // ★ Composite 用
    uint32_t GetForwardColorArraySrv() const { return targetArray_->GetColorSrv(); }

    uint32_t GetForwardDepthArraySrv() const { return targetArray_->GetDepthSrv(); }

    uint32_t GetLayerCount() const { return (uint32_t)BlendCategory::Count; }
private:
    std::unique_ptr<ForwardRenderTargetArray> targetArray_;

	std::array<std::unique_ptr<ForwardRenderPath>, (int)BlendCategory::Count> paths_;
};

