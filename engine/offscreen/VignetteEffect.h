#pragma once
#include "BaseOffScreen.h"
class VignetteEffect : public BaseOffScreen
{
public:
	VignetteEffect();
	~VignetteEffect();

	// 更新
	void Update() override;
	// 描画
	void Draw() override;

private:
	void CreateEffectResource();

private:
	struct VignetteEffectData {
		float radius; // ヴィネットの開始距離
		float intensity; // 暗くなる強さ
		float softness; // 中心から端へのフェードの滑らかさ
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> effectResource_ = nullptr;
	VignetteEffectData* effectData_ = nullptr;
};

