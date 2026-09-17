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
	// 最終的な出力先のRTV。ブルームのように途中で自前のRTへ描くエフェクトが、
	// 最後にチェーンの出力先へ戻すために使う（PostEffectPath が Draw の直前に渡す）
	virtual void SetOutputRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv) { outputRtv_ = rtv; };
	bool IsActive() const { return isActive_; };
	// 派生側に同じものが散らばっていたのでここへ集約した。
	// エディタは BaseOffScreen* しか持たないので、基底に無いと切り替えられない
	void SetActive(bool flag) { isActive_ = flag; }
	std::string GetName() { return name_; }
protected:
	DirectXManager* dxManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;

	bool isActive_ = false;
	D3D12_GPU_DESCRIPTOR_HANDLE inputSrv_;
	D3D12_CPU_DESCRIPTOR_HANDLE outputRtv_{};
	std::string name_;
};

