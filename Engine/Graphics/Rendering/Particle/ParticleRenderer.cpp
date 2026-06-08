#include "ParticleRenderer.h"

void ParticleRenderer::Draw(ID3D12GraphicsCommandList* commandList, size_t instanceSize, uint32_t indexCount)
{
    commandList->DrawIndexedInstanced(indexCount, static_cast<UINT>(instanceSize), 0, 0, 0);
}
