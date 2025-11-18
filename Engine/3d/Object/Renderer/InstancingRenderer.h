#pragma once
#include "BaseRenderer.h"
#include <vector>
#include <DirectXTex/DirectXTex.h>
#include <wrl.h>
#include <d3d12.h>
#include <math/Vector4.h>
#include "3d/Object/Model/Model.h"
#include "3d/Object/Renderer/PrimitiveRenderer.h"
#include "base/Particle/ParticleStruct.h"

class InstancingRenderer : public BaseRenderer
{
public:
    InstancingRenderer(const std::string& renderName, PrimitiveType type, const std::string& texturePath);

    void Update(WorldTransform* parentTransform) override;
    void Draw() override;
    // GBufferに描画
    void DrawGBuffer() override;
#ifdef _DEBUG
    void DebugGui(size_t index) override;
#endif

    WorldTransform* GetWorldTransform() const override;
    Model* GetModel() const override;

    void SetInstanceList(const std::vector<InstanceData>& instances);

private:
    void CreateInstanceBuffer();

    std::vector<InstanceData> instances_;
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    InstanceData* mappedInstanceData_ = nullptr;

    D3D12_VERTEX_BUFFER_VIEW vbView_;

    std::unique_ptr<Model> model_;
    std::string texturePath_;
};
