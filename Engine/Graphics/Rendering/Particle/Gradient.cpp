#include "Gradient.h"
#include <algorithm>

Vector4 Gradient::Evaluate(float t) const
{
	// 未設定。呼び出し側で無視できるよう、掛けても影響しない白を返す
	if (keys_.empty()) {
		return Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
	if (keys_.size() == 1) {
		return keys_.front().color;
	}

	t = std::clamp(t, 0.0f, 1.0f);

	if (t <= keys_.front().time) {
		return keys_.front().color;
	}
	if (t >= keys_.back().time) {
		return keys_.back().color;
	}

	for (size_t i = 0; i + 1 < keys_.size(); ++i) {
		const GradientKey& a = keys_[i];
		const GradientKey& b = keys_[i + 1];
		if (t < a.time || t > b.time) {
			continue;
		}

		const float span = b.time - a.time;
		// 同じ時刻にキーが重なっている場合は後ろのキーへ切り替わる（色の切り替え表現）
		if (span <= 0.0f) {
			return b.color;
		}
		const float ratio = (t - a.time) / span;
		return a.color + (b.color - a.color) * ratio;
	}

	return keys_.back().color;
}

void Gradient::SetKeys(std::vector<GradientKey> keys)
{
	keys_ = std::move(keys);
	SortKeys();
}

void Gradient::SortKeys()
{
	std::stable_sort(keys_.begin(), keys_.end(),
		[](const GradientKey& a, const GradientKey& b) { return a.time < b.time; });
}
