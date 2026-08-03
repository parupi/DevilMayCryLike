#include "Curve.h"
#include <algorithm>

float Curve::Evaluate(float t) const
{
	// カーブ未設定。呼び出し側で無視できるよう、掛けても影響しない 1.0 を返す
	if (keys_.empty()) {
		return 1.0f;
	}
	if (keys_.size() == 1) {
		return keys_.front().value;
	}

	t = std::clamp(t, 0.0f, 1.0f);

	// 端の外側は端の値で一定にする
	if (t <= keys_.front().time) {
		return keys_.front().value;
	}
	if (t >= keys_.back().time) {
		return keys_.back().value;
	}

	// t を含む区間を探して線形補間する
	for (size_t i = 0; i + 1 < keys_.size(); ++i) {
		const CurveKey& a = keys_[i];
		const CurveKey& b = keys_[i + 1];
		if (t < a.time || t > b.time) {
			continue;
		}

		const float span = b.time - a.time;
		// 同じ時刻にキーが重なっている場合は後ろのキーへ切り替わる（段差表現）
		if (span <= 0.0f) {
			return b.value;
		}
		const float ratio = (t - a.time) / span;
		return a.value + (b.value - a.value) * ratio;
	}

	return keys_.back().value;
}

void Curve::SetKeys(std::vector<CurveKey> keys)
{
	keys_ = std::move(keys);
	SortKeys();
}

void Curve::SortKeys()
{
	std::stable_sort(keys_.begin(), keys_.end(),
		[](const CurveKey& a, const CurveKey& b) { return a.time < b.time; });
}

Curve Curve::Linear(float from, float to)
{
	Curve curve;
	curve.keys_ = { { 0.0f, from }, { 1.0f, to } };
	return curve;
}

Curve Curve::Constant(float value)
{
	Curve curve;
	curve.keys_ = { { 0.0f, value } };
	return curve;
}
