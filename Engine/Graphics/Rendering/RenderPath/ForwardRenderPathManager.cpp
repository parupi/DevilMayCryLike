#include "ForwardRenderPathManager.h"

void ForwardRenderPathManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
    // ==============================
    // Texture2DArray を生成
    // ==============================
    targetArray_ = std::make_unique<ForwardRenderTargetArray>();
    targetArray_->Initialize(dxManager, static_cast<uint32_t>(BlendCategory::Count));

    // ==============================
    // BlendCategory ごとの Path
    // ==============================
    for (int i = 0; i < (int)BlendCategory::Count; ++i)
    {
        auto path = std::make_unique<ForwardRenderPath>();

        path->Initialize(dxManager, psoManager, targetArray_.get(), static_cast<uint32_t>(i));

        paths_[i] = std::move(path);
    }
}
