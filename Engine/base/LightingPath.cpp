#include "LightingPath.h"
#include "GBufferManager.h"
#include "base/DirectXManager.h"
#include <offscreen/OffScreenManager.h>

void LightingPath::Initialize(DirectXManager* dx, GBufferManager* gBuffer, PSOManager* psoManager)
{
    dxManager_ = dx;
    gBufferManager_ = gBuffer;
    psoManager_ = psoManager;

    CreateFullScreenVB();
}

void LightingPath::CreateFullScreenVB()
{
    // フルスクリーンクアッド（左下原点の場合）
    FullScreenVertex vertices[3] = {
        {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // 左上
        {{ 3.0f, 1.0f, 0.0f}, {2.0f, 0.0f}}, // 右上
        {{ -1.0f, -3.0f, 0.0f}, {0.0f, 2.0f}}, // 左下
    };

    const UINT vbSize = sizeof(vertices);

    // バッファを作成
    dxManager_->CreateBufferResource(vbSize, fullScreenVB_);

    // map
    FullScreenVertex* mapped = nullptr;
    fullScreenVB_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
    memcpy(mapped, vertices, vbSize);
    fullScreenVB_->Unmap(0, nullptr);

    // VBV
    fullScreenVBV_.BufferLocation = fullScreenVB_->GetGPUVirtualAddress();
    fullScreenVBV_.SizeInBytes = vbSize;
    fullScreenVBV_.StrideInBytes = sizeof(FullScreenVertex);
}

void LightingPath::Begin()
{
    auto cmd = dxManager_->GetCommandList();

    //// BackBufferにRTV設定
    dxManager_->SetMainRTV();
    dxManager_->SetMainDepth(nullptr);

    cmd->SetPipelineState(psoManager_->GetLightingPathPSO());
    cmd->SetGraphicsRootSignature(psoManager_->GetLightingPathSignature());

    gBufferManager_->SetGBufferSRVs();
}

void LightingPath::DrawDirectionalLight()
{
    auto cmd = dxManager_->GetCommandList();

    // FullScreenQuad 描画
    cmd->IASetVertexBuffers(0, 1, &fullScreenVBV_);
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmd->DrawInstanced(3, 1, 0, 0);
}

void LightingPath::End()
{
}
