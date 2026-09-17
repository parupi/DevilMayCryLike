#pragma once
#include "BaseOffScreen.h"
class SmoothEffect : public BaseOffScreen
{
public:
	SmoothEffect();
	~SmoothEffect();

	// 更新
	void Update() override;
	// 描画
	void Draw() override;

	struct SmoothEffectData {
		float blurStrength; // ぼかしの強さ
		int iterations; // ブラーの回数
	};
	// 定数バッファへの直書きポインタ。エディタが編集する
	SmoothEffectData* GetEffectData() { return effectData_; }

private:
	// エフェクトの情報を入れるためのリソース生成
	void CreateEffectResource();

	uint32_t effectHandle_ = 0;
	SmoothEffectData* effectData_ = nullptr;
};

