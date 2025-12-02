#pragma once
#include <memory>
#include <d3d12.h>

class DirectXManager;
class GBufferManager;
class PSOManager;

class GBufferPath
{
public:
	GBufferPath() = default;
	~GBufferPath();

	void Initialize(DirectXManager* dxManager, GBufferManager* gBuffer, PSOManager* psoManager);

	void Begin();

	void Draw();

	void End();
private:
	DirectXManager* dxManager_ = nullptr;
	GBufferManager* gBuffer_ = nullptr;
	PSOManager* psoManager_ = nullptr;
};

