#pragma once
#include "BaseOffScreen.h"


class GrayEffect : public BaseOffScreen
{
public:
	/// <param name="name">OffScreenManager::FindEffect で引くための名前。
	/// 名前が無いと同名扱いで2つ目以降が弾かれるので、必ず付けること</param>
	explicit GrayEffect(const std::string& name = "Gray");
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

