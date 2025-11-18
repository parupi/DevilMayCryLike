#include "InstancingRenderer.h"
#include "RendererManager.h"
#include "PrimitiveFactory.h"
#include <cassert>
#include <cstring>
#include <3d/SkySystem/SkySystem.h>
#include <base/TextureManager.h>

InstancingRenderer::InstancingRenderer(const std::string& renderName, PrimitiveType type, const std::string& texturePath)
{
    name_ = renderName;
    texturePath_ = texturePath;

    localTransform_ = std::make_unique<WorldTransform>();
    localTransform_->Initialize();

    model_ = PrimitiveFactory::Create(type, texturePath);
    
    CreateInstanceBuffer();
}

void InstancingRenderer::CreateInstanceBuffer()
{
    auto* dxManager = RendererManager::GetInstance()->GetDxManager();
    size_t bufferSize = sizeof(InstanceData) * 1024; // 1024 instance max

    dxManager->CreateBufferResource(bufferSize, instanceBuffer_);
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData_));

    vbView_.BufferLocation = instanceBuffer_->GetGPUVirtualAddress();
    vbView_.StrideInBytes = sizeof(InstanceData);
    vbView_.SizeInBytes = static_cast<UINT>(bufferSize);
}

void InstancingRenderer::SetInstanceList(const std::vector<InstanceData>& instances)
{
    assert(instances.size() <= 1024);
    instances_ = instances;
    std::memcpy(mappedInstanceData_, instances.data(), sizeof(InstanceData) * instances.size());
}

void InstancingRenderer::Update(WorldTransform* parentTransform)
{
    if (localTransform_->GetParent() == nullptr) {
        localTransform_->SetParent(parentTransform);
    }
    localTransform_->TransferMatrix();

    Matrix4x4 wvp = localTransform_->GetMatWorld() * CameraManager::GetInstance()->GetActiveCamera()->GetViewProjectionMatrix();
    localTransform_->SetMapWVP(wvp);
    localTransform_->SetMapWorld(localTransform_->GetMatWorld());
}

void InstancingRenderer::Draw()
{
    if (instances_.empty()) return;

    auto* commandList = RendererManager::GetInstance()->GetDxManager()->GetCommandList();
    // インスタンシング描画
    commandList->DrawInstanced(6, static_cast<UINT>(instances_.size()), 0, 0);
}

void InstancingRenderer::DrawGBuffer()
{
}

#ifdef _DEBUG
void InstancingRenderer::DebugGui(size_t index)
{
    std::string label = "InstancingRender" + std::to_string(index);
    if (ImGui::TreeNode(label.c_str())) {
        localTransform_->DebugGui();
        model_->DebugGuiPrimitive();
        ImGui::TreePop();
    }
}
#endif

WorldTransform* InstancingRenderer::GetWorldTransform() const {
    return localTransform_.get();
}

Model* InstancingRenderer::GetModel() const {
    return model_.get();
}
