#pragma once
#include "BaseOffScreen.h"


class GrayEffect : public BaseOffScreen
{
public:
	GrayEffect();
	~GrayEffect();

	// 更新
	void Update() override;
	// 描画
	void Draw() override;

private:
	void CreateEffectResource();

private:
	struct GrayEffectData {
		float intensity;
	};

	//Microsoft::WRL::ComPtr<ID3D12Resource> effectResource_ = nullptr;
	uint32_t effectHandle_ = 0;
	GrayEffectData* effectData_ = nullptr;
};

