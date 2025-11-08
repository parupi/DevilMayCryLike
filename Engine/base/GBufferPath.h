#pragma once
#include <memory>

class DirectXManager;
class GBufferManager;

class GBufferPath
{
public:
	void Initialize(DirectXManager* dxManager, GBufferManager* gBuffer);

	void Begin();

	void End();

private:
	DirectXManager* dxManager_ = nullptr;
	GBufferManager* gBuffer_ = nullptr;
};

