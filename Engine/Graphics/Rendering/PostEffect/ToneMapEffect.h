#pragma once
#include "BaseOffScreen.h"

/// <summary>
/// 露出とトーンマップ（HDR → 0〜1）。ブルームの直後に置く常設のパス（MyGameTitle が登録）。
/// </summary>
class ToneMapEffect : public BaseOffScreen {
public:
	explicit ToneMapEffect(const std::string& name);
	~ToneMapEffect();

	void Update() override;
	void Draw()   override;

private:
	// HLSL の ToneMapParam と並びを合わせること（ToneMap.PS.hlsl）
	struct ToneMapData {
		float exposure = 1.0f;
		float shoulderStart = 0.6f;
		float _pad0 = 0.0f;
		float _pad1 = 0.0f;
	};

	uint32_t effectHandle_ = 0;
	ToneMapData* effectDataPtr_ = nullptr;
};
