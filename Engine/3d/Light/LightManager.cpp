#include "LightManager.h"
#include <cassert>
#include <mutex>

LightManager* LightManager::instance = nullptr;
std::once_flag LightManager::initInstanceFlag;

LightManager* LightManager::GetInstance()
{
    std::call_once(initInstanceFlag, []() {
        instance = new LightManager();
        });
    return instance;
}

void LightManager::Initialize(DirectXManager* dxManager)
{
    dxManager_ = dxManager;
    CreateLightBuffers();
}

void LightManager::Finalize()
{
    lights_.clear();
    gpuLightCache_.clear();

    // ResourceManager に破棄任せる
    lightBufferHandle_ = 0;
    lightCountHandle_ = 0;

    mappedLightPtr_ = nullptr;
    mappedCountPtr_ = nullptr;

    dxManager_ = nullptr;

    delete instance;
    instance = nullptr;
}

void LightManager::CreateLightBuffers()
{
    auto* rm = dxManager_->GetResourceManager();

    // --- 1. Light StructuredBuffer (UPLOAD) ---
    lightBufferHandle_ = rm->CreateUploadBuffer(sizeof(LightData) * MaxLights, L"LightData");

    // 永続 Map
    mappedLightPtr_ = reinterpret_cast<LightData*>(rm->Map(lightBufferHandle_));

    // --- 2. LightCount ConstantBuffer (UPLOAD) ---
    UINT cbSize = (sizeof(UINT) + 255) & ~255;

    lightCountHandle_ = rm->CreateUploadBuffer(cbSize, L"LightCount");

    mappedCountPtr_ = reinterpret_cast<UINT*>(rm->Map(lightCountHandle_));

    // --- 3. SRV 作成 ---
    auto* srv = dxManager_->GetSrvManager();
    srvIndex_ = srv->Allocate();

    srv->CreateSRVforStructuredBuffer(srvIndex_, rm->GetResource(lightBufferHandle_), MaxLights, sizeof(LightData));
}

void LightManager::Update()
{
    gpuLightCache_.clear();
    gpuLightCache_.reserve(lights_.size());

    for (auto& light : lights_) {
        light->Update();
        gpuLightCache_.push_back(light->GetLightData());
    }

    // --- GPU バッファ更新（永続 Map なので memcpy だけ） ---
    assert(gpuLightCache_.size() <= MaxLights);

    memcpy(
        mappedLightPtr_,
        gpuLightCache_.data(),
        sizeof(LightData) * gpuLightCache_.size()
    );

    *mappedCountPtr_ = static_cast<UINT>(gpuLightCache_.size());
}

void LightManager::AddLight(std::unique_ptr<BaseLight> light)
{
    lights_.push_back(std::move(light));
}

void LightManager::DeleteAllLight()
{
    lights_.clear();
}

void LightManager::BindLightsToShader()
{
    auto* cmd = dxManager_->GetCommandList();
    auto* srv = dxManager_->GetSrvManager();
    auto* rm = dxManager_->GetResourceManager();

    // Count CBV
    cmd->SetGraphicsRootConstantBufferView(
        2,
        rm->GetGPUVirtualAddress(lightCountHandle_)
    );

    // SRV (StructuredBuffer)
    cmd->SetGraphicsRootDescriptorTable(
        3,
        srv->GetGPUDescriptorHandle(srvIndex_)
    );
}
