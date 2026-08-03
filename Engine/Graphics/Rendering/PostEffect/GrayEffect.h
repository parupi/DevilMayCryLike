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

	struct GrayEffectData {
		float intensity;
	};
	// 定数バッファへの直書きポインタ。エディタが編集する
	GrayEffectData* GetEffectData() { return effectData_; }

private:
	void CreateEffectResource();

private:
	uint32_t effectHandle_ = 0;
	GrayEffectData* effectData_ = nullptr;
};

