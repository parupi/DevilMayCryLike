#include "ParticleRenderer.h"

void ParticleRenderer::Draw(ID3D12GraphicsCommandList* commandList, size_t instanceSize)
{
    // インスタンシング描画
    commandList->DrawInstanced(6, static_cast<UINT>(instanceSize), 0, 0);
}
