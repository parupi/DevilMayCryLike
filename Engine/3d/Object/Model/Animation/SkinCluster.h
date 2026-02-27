#pragma once
#include <3d/Object/Model/ModelStructs.h>

class DirectXManager;
class SrvManager;

class SkinCluster
{
public:
    SkinCluster() = default;
    ~SkinCluster();

    void Initialize(
        const SkeletonData& skeleton,
        const SkinnedMeshData& meshData,
        const std::map<std::string, JointWeightData>& skinClusterData,
        DirectXManager* dxManager,
        SrvManager* srvManager);

    void CreateSkinCluster(
        const SkeletonData& skeleton,
        const SkinnedMeshData& meshData,
        const std::map<std::string, JointWeightData>& skinClusterData);

    void UpdateSkinning();
    void UpdateInputVertex(const SkinnedMeshData& meshData);
    void UpdateSkinCluster(const SkeletonData& skeleton);

    const D3D12_VERTEX_BUFFER_VIEW& GetOutputVBV() const { return outputVBV_; }

private:
    void CreatePalette(const SkeletonData& skeleton);
    void CreateInfluence(const SkinnedMeshData& meshData);
    void CreateInputVertex();
    void CreateOutputVertex();
    void CreateSkinningInfo();

private:
    DirectXManager* dxManager_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    uint32_t vertexCount_ = 0;

    // ---- Handles ----
    uint32_t paletteHandle_;
    uint32_t influenceHandle_;
    uint32_t inputVertexHandle_;
    uint32_t outputVertexHandle_;
    uint32_t skinningInfoHandle_;

    // ---- CPU-side mapped pointers ----
    WellForGPU* mappedPalette_ = nullptr;
    VertexInfluence* mappedInfluence_ = nullptr;
    VertexData* mappedInputVertex_ = nullptr;
    SkinningInformation* skinningInfoData_ = nullptr;

    // ---- Views / Handles ----
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSRV_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> influenceSRV_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> inputVertexSRV_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputVertexUAV_;

    D3D12_VERTEX_BUFFER_VIEW outputVBV_{};
};
