#include "SkinCluster.h"
#include "base/DirectXManager.h"
#include "base/SrvManager.h"
#include "base/ResourceManager.h"

SkinCluster::~SkinCluster()
{
    // Upload バッファは Unmap 不要（ResourceManager が永続マップを保持）
    mappedPalette_ = nullptr;
    mappedInfluence_ = nullptr;
    mappedInputVertex_ = nullptr;
    skinningInfoData_ = nullptr;
}

void SkinCluster::Initialize(
    const SkeletonData& skeleton,
    const SkinnedMeshData& meshData,
    const std::map<std::string, JointWeightData>& skinClusterData,
    DirectXManager* dxManager,
    SrvManager* srvManager)
{
    dxManager_ = dxManager;
    srvManager_ = srvManager;

    vertexCount_ = static_cast<uint32_t>(meshData.vertices.size());

    CreateSkinCluster(skeleton, meshData, skinClusterData);
}

void SkinCluster::CreateSkinCluster(
    const SkeletonData& skeleton,
    const SkinnedMeshData& meshData,
    const std::map<std::string, JointWeightData>& skinClusterData)
{
    CreatePalette(skeleton);
    CreateInfluence(meshData);
    CreateInputVertex();
    CreateOutputVertex();
    CreateSkinningInfo();

    // --- inverse bind pose ---
    std::vector<Matrix4x4> invBind(skeleton.joints.size(), MakeIdentity4x4());

    for (auto& it : skinClusterData)
    {
        auto found = skeleton.jointMap.find(it.first);
        if (found == skeleton.jointMap.end()) continue;

        uint32_t jointIdx = found->second;
        invBind[jointIdx] = it.second.inverseBindPoseMatrix;

        for (auto& vw : it.second.vertexWeights)
        {
            auto& inf = mappedInfluence_[vw.vertexIndex];
            for (uint32_t i = 0; i < kNumMaxInfluence; i++)
            {
                if (inf.weights[i] == 0.0f)
                {
                    inf.weights[i] = vw.weight;
                    inf.jointIndices[i] = jointIdx;
                    break;
                }
            }
        }
    }

    // inverse bind → palette に反映（UpdateSkinCluster() で実際の行列更新）
    for (size_t i = 0; i < invBind.size(); i++)
    {
        mappedPalette_[i].skeletonSpaceInverseTransposeMatrix = invBind[i];
    }
}

void SkinCluster::CreatePalette(const SkeletonData& skeleton)
{
    size_t size = sizeof(WellForGPU) * skeleton.joints.size();
    paletteHandle_ = dxManager_->GetResourceManager()->CreateUploadBuffer(size, L"Palette");

    mappedPalette_ = reinterpret_cast<WellForGPU*>(
        dxManager_->GetResourceManager()->Map(paletteHandle_));

    uint32_t index = srvManager_->Allocate();
    paletteSRV_.first = srvManager_->GetCPUDescriptorHandle(index);
    paletteSRV_.second = srvManager_->GetGPUDescriptorHandle(index);

    auto res = dxManager_->GetResourceManager()->GetResource(paletteHandle_);
    srvManager_->CreateSRVforStructuredBuffer(index, res, (UINT)skeleton.joints.size(), sizeof(WellForGPU));
}

void SkinCluster::CreateInfluence(const SkinnedMeshData& meshData)
{
    size_t size = sizeof(VertexInfluence) * meshData.vertices.size();
    influenceHandle_ = dxManager_->GetResourceManager()->CreateUploadBuffer(size, L"Influence");

    mappedInfluence_ = reinterpret_cast<VertexInfluence*>(
        dxManager_->GetResourceManager()->Map(influenceHandle_));

    uint32_t index = srvManager_->Allocate();
    influenceSRV_.first = srvManager_->GetCPUDescriptorHandle(index);
    influenceSRV_.second = srvManager_->GetGPUDescriptorHandle(index);

    auto res = dxManager_->GetResourceManager()->GetResource(influenceHandle_);
    srvManager_->CreateSRVforStructuredBuffer(index, res, (UINT)meshData.vertices.size(), sizeof(VertexInfluence));
}

void SkinCluster::CreateInputVertex()
{
    size_t size = sizeof(VertexData) * vertexCount_;
    inputVertexHandle_ = dxManager_->GetResourceManager()->CreateUploadBuffer(size, L"InputVertex");

    mappedInputVertex_ = reinterpret_cast<VertexData*>(
        dxManager_->GetResourceManager()->Map(inputVertexHandle_));

    uint32_t index = srvManager_->Allocate();
    inputVertexSRV_.first = srvManager_->GetCPUDescriptorHandle(index);
    inputVertexSRV_.second = srvManager_->GetGPUDescriptorHandle(index);

    auto res = dxManager_->GetResourceManager()->GetResource(inputVertexHandle_);
    srvManager_->CreateSRVforStructuredBuffer(index, res, vertexCount_, sizeof(VertexData));
}

void SkinCluster::CreateOutputVertex()
{
    size_t size = sizeof(VertexData) * vertexCount_;
    outputVertexHandle_ = dxManager_->GetResourceManager()->CreateDefaultBuffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, L"OutputVertex");

    uint32_t index = srvManager_->Allocate();
    outputVertexUAV_.first = srvManager_->GetCPUDescriptorHandle(index);
    outputVertexUAV_.second = srvManager_->GetGPUDescriptorHandle(index);

    auto res = dxManager_->GetResourceManager()->GetResource(outputVertexHandle_);
    srvManager_->CreateUAVforStructuredBuffer(index, res, vertexCount_, sizeof(VertexData));

    outputVBV_.BufferLocation = res->GetGPUVirtualAddress();
    outputVBV_.SizeInBytes = (UINT)size;
    outputVBV_.StrideInBytes = sizeof(VertexData);
}

void SkinCluster::CreateSkinningInfo()
{
    skinningInfoHandle_ =
        dxManager_->GetResourceManager()->CreateUploadBuffer(sizeof(SkinningInformation), L"SkinningInfo");

    skinningInfoData_ =
        reinterpret_cast<SkinningInformation*>(dxManager_->GetResourceManager()->Map(skinningInfoHandle_));

    skinningInfoData_->vertexCount = vertexCount_;
}

void SkinCluster::UpdateInputVertex(const SkinnedMeshData& meshData)
{
    for (size_t i = 0; i < meshData.vertices.size(); i++)
    {
        mappedInputVertex_[i] = meshData.vertices[i];
    }
}

void SkinCluster::UpdateSkinCluster(const SkeletonData& skeleton)
{
    for (uint32_t i = 0; i < skeleton.joints.size(); i++)
    {
        auto mat = mappedPalette_[i].skeletonSpaceInverseTransposeMatrix *
            skeleton.joints[i].skeletonSpaceMatrix;

        mappedPalette_[i].skeletonSpaceMatrix = mat;
        mappedPalette_[i].skeletonSpaceInverseTransposeMatrix = Transpose(Inverse(mat));
    }
}

void SkinCluster::UpdateSkinning()
{
    auto* cmd = dxManager_->GetCommandList();

    // UAV バリア（初回）
    {
        auto res = dxManager_->GetResourceManager()->GetResource(outputVertexHandle_);
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = res;
        cmd->ResourceBarrier(1, &barrier);
    }

    cmd->SetComputeRootDescriptorTable(0, paletteSRV_.second);
    cmd->SetComputeRootDescriptorTable(1, inputVertexSRV_.second);
    cmd->SetComputeRootDescriptorTable(2, influenceSRV_.second);
    cmd->SetComputeRootDescriptorTable(3, outputVertexUAV_.second);

    auto addr = dxManager_->GetResourceManager()->GetResource(skinningInfoHandle_)->GetGPUVirtualAddress();
    cmd->SetComputeRootConstantBufferView(4, addr);

    uint32_t numGroups = (vertexCount_ + 1023) / 1024;
    cmd->Dispatch(numGroups, 1, 1);

    // UAV → VBV の遷移
    {
        auto res = dxManager_->GetResourceManager()->GetResource(outputVertexHandle_);
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = res;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        cmd->ResourceBarrier(1, &barrier);
    }
}
