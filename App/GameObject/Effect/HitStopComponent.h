#pragma once

class HitStopComponent
{
public:
	HitStopComponent() = default;
	~HitStopComponent() = default;
	// sceneDtとhitStopを掛け合わせる
	void Update(float sceneDt);
	// 適用
	void Apply(float duration, float scale = 0.0f);
	// 計算したスケールを取得
	float GetScale() const { return localScale_; }
private:
	float timer_ = 0.0f;
	float localScale_ = 1.0f;
	float stopScale_ = 0.0f;
};

