#include "PrimitiveRenderer.h"
#include "PrimitiveFactory.h"
#include "RendererManager.h"
#include <3d/SkySystem/SkySystem.h>
#include <base/TextureManager.h>

PrimitiveRenderer::PrimitiveRenderer(const std::string& renderName, PrimitiveType type, std::string textureName) {
    name_ = renderName;
    localTransform_ = std::make_unique<WorldTransform>();
    localTransform_->Initialize();

    // Primitiveモデルを生成
    model_ = PrimitiveFactory::Create(type, textureName); // Plane, Ring, Cylinder を返す
}

PrimitiveRenderer::~PrimitiveRenderer()
{
    model_.reset();
    localTransform_.reset();
}

void PrimitiveRenderer::Update(WorldTransform* parentTransform) {
    Camera* camera = CameraManager::GetInstance()->GetActiveCamera();
    if (!camera) return;

    model_->Update();

    if (localTransform_->GetParent() == nullptr) {
        localTransform_->SetParent(parentTransform);
    }
    localTransform_->TransferMatrix();

    Matrix4x4 wvp = localTransform_->GetMatWorld() * camera->GetViewProjectionMatrix();
    localTransform_->SetMapWVP(wvp);
    localTransform_->SetMapWorld(localTransform_->GetMatWorld());
}

void PrimitiveRenderer::Draw() {
    RendererManager::GetInstance()->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(1, localTransform_->GetConstBuffer()->GetGPUVirtualAddress());

    // 環境マップバインド
    int envMapIndex = SkySystem::GetInstance()->GetEnvironmentMapIndex();
   
    if (envMapIndex < 0) {
        TextureManager::GetInstance()->LoadTexture("skybox_cube.dds");
        envMapIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("skybox_cube.dds");
    }
    RendererManager::GetInstance()->GetSrvManager()->SetGraphicsRootDescriptorTable(7, envMapIndex);

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