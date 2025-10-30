#pragma once
#include "BaseOffScreen.h"
class VignetteEffect : public BaseOffScreen
{
public:
	VignetteEffect(const std::string& name);
	~VignetteEffect();

	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	
private:
	struct VignetteEffectData {
		float radius; // ヴィネットの開始距離
		float intensity; // 暗くなる強さ
		float softness; // 中心から端へのフェードの滑らかさ
	};

public:
	VignetteEffectData& GetEffectData() { return effectData_; }
	void SetActive(bool flag) { isActive_ = flag; }
private:
	void CreateEffectResource();

	Microsoft::WRL::ComPtr<ID3D12Resource> effectResource_ = nullptr;
	VignetteEffectData effectData_;
	VignetteEffectData* effectDataPtr_;
};

