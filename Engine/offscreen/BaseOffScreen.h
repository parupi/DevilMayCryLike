#pragma once
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"
class BaseOffScreen
{
public:
	virtual ~BaseOffScreen() = default;

	virtual void Update() = 0;
	virtual void Draw() = 0;

	virtual void SetInputTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle) { inputSrv_ = handle; };
	bool IsActive() const { return isActive_; };
	std::string GetName() { return name_; }
protected:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	bool isActive_ = false;
	D3D12_GPU_DESCRIPTOR_HANDLE inputSrv_;
	std::string name_;
};

