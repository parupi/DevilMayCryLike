#pragma once
#include "BaseOffScreen.h"


class GrayEffect : public BaseOffScreen
{
public:
	GrayEffect();
	~GrayEffect() = default;

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

	Microsoft::WRL::ComPtr<ID3D12Resource> effectResource_ = nullptr;
	GrayEffectData* effectData_ = nullptr;
};

