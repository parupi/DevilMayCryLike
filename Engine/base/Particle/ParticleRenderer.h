#pragma once
#include <string>
#include <d3d12.h>

class ParticleRenderer
{
public:
	void Draw(ID3D12GraphicsCommandList* commandList, size_t instanceSize);
};
