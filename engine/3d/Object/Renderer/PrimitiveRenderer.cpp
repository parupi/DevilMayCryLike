#include "PrimitiveRenderer.h"
#include "PrimitiveFactory.h"
#include "RendererManager.h"

PrimitiveRenderer::PrimitiveRenderer(const std::string& renderName, PrimitiveType type, std::string textureName) {
    name = renderName;
    localTransform_ = std::make_unique<WorldTransform>();
    localTransform_->Initialize();

    // Primitiveモデルを生成
    model_ = PrimitiveFactory::Create(type, textureName); // Plane, Ring, Cylinder を返す
}

void PrimitiveRenderer::Update(WorldTransform* parentTransform) {
    model_->Update();

    if (localTransform_->GetParent() == nullptr) {
        localTransform_->SetParent(parentTransform);
    }
    localTransform_->TransferMatrix();

    Matrix4x4 wvp = localTransform_->GetMatWorld() * camera_->GetViewProjectionMatrix();
    localTransform_->SetMapWVP(wvp);
    localTransform_->SetMapWorld(localTransform_->GetMatWorld());
}

void PrimitiveRenderer::Draw(WorldTransform* parentTransform) {
    RendererManager::GetInstance()->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(1, localTransform_->GetConstBuffer()->GetGPUVirtualAddress());

    model_->Draw();
}

#ifdef _DEBUG
void PrimitiveRenderer::DebugGui(size_t index)
{
    std::string label = "TransformRender" + std::to_string(index);
    if (ImGui::TreeNode(label.c_str())) {
        localTransform_->DebugGui();
        model_->DebugGuiPrimitive();
        ImGui::TreePop();
    }
}
#endif