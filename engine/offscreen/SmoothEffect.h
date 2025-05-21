#pragma once
#include "BaseOffScreen.h"
class SmoothEffect : public BaseOffScreen
{
public:
	SmoothEffect();
	~SmoothEffect() = default;

	// 更新
	void Update() override;
	// 描画
	void Draw() override;

private:
	void CreateEffectResource();

private:
	struct SmoothEffectData {
		float blurStrength; // ぼかしの強さ
		int iterations; // ブラーの回数
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> effectResource_ = nullptr;
	SmoothEffectData* effectData_ = nullptr;
};

