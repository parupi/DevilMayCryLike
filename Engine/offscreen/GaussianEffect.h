#pragma once
#include "BaseOffscreen.h"
#include <math/Vector2.h>
class GaussianEffect : public BaseOffScreen
{
public:
	GaussianEffect();
	~GaussianEffect();

	// 更新
	void Update() override;
	// 描画
	void Draw() override;

private:
	// エフェクトの情報を入れるためのリソース生成
	void CreateEffectResource();

	struct alignas(16) GaussianEffectData {
		float sigma;           // ガウス分布のσ
		float blurStrength;    // ブラーの強さ倍率
		float alphaMode;       // 0 = 固定1.0, 1 = サンプル結果適用
		float padding0;
		Vector2 uvClampMin;     // UV Clamp最小値（例：0.0f, 0.0f）
		Vector2 uvClampMax;     // UV Clamp最大値（例：1.0f, 1.0f）
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> effectResource_ = nullptr;
	GaussianEffectData* effectData_ = nullptr;
};

