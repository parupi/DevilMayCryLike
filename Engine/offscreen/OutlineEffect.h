#pragma once
#include "BaseOffScreen.h"
class OutlineEffect : public BaseOffScreen
{
public:
	OutlineEffect();
	~OutlineEffect();

	// 更新
	void Update() override;
	// 描画
	void Draw() override;

private:
	// エフェクトの情報を入れるためのリソース生成
	void CreateEffectResource();

	struct OutlineEffectData {

	};

	Microsoft::WRL::ComPtr<ID3D12Resource> effectResource_ = nullptr;
	OutlineEffectData* effectData_ = nullptr;
};

