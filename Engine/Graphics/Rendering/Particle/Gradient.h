#pragma once
#include <vector>
#include "Math/Vector4.h"

/// <summary>グラデーション上の1点。time は 0〜1 に正規化した寿命進行度</summary>
struct GradientKey {
	float time;
	Vector4 color;
};

/// <summary>
/// 時間による色の変化を表すグラデーション。
///
/// 「白 → 黄 → オレンジ → 透明」のような火花の色遷移を表現するために使う。
/// キーが無い場合は「未設定」とみなして白(1,1,1,1)を返すので、
/// 呼び出し側は IsEmpty() で従来挙動へフォールバックできる。
/// </summary>
class Gradient
{
public:
	Gradient() = default;

	/// <summary>t(0〜1)における色を取得する。キーが無い場合は白</summary>
	Vector4 Evaluate(float t) const;

	/// <summary>キーが1つも無い（＝未設定）か</summary>
	bool IsEmpty() const { return keys_.empty(); }

	/// <summary>キーを差し替える（時刻順に整列される）</summary>
	void SetKeys(std::vector<GradientKey> keys);
	void Clear() { keys_.clear(); }

	const std::vector<GradientKey>& GetKeys() const { return keys_; }
	/// <summary>エディタからの直接編集用。編集後は SortKeys() を呼ぶこと</summary>
	std::vector<GradientKey>& GetKeysForEdit() { return keys_; }
	/// <summary>キーを時刻順に並べ直す</summary>
	void SortKeys();

private:
	// time の昇順で保持する
	std::vector<GradientKey> keys_;
};
