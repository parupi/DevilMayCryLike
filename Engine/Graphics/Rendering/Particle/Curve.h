#pragma once
#include <vector>

/// <summary>カーブ上の1点。time は 0〜1 に正規化した寿命進行度</summary>
struct CurveKey {
	float time;
	float value;
};

/// <summary>
/// 時間によるスカラー値の変化を表すカーブ。
///
/// パーティクルの寿命進行度 t（0=発生, 1=消滅）を渡して値を得る。
/// キーが無い場合は「カーブ未設定」とみなして 1.0f を返すので、
/// 呼び出し側は IsEmpty() で従来挙動へフォールバックできる。
///
/// キー間は線形補間。両端の外側はそれぞれ端のキーの値で一定になる。
/// </summary>
class Curve
{
public:
	Curve() = default;

	/// <summary>t(0〜1)における値を取得する。キーが無い場合は 1.0f</summary>
	float Evaluate(float t) const;

	/// <summary>キーが1つも無い（＝カーブ未設定）か</summary>
	bool IsEmpty() const { return keys_.empty(); }

	/// <summary>キーを差し替える（時刻順に整列される）</summary>
	void SetKeys(std::vector<CurveKey> keys);
	void Clear() { keys_.clear(); }

	const std::vector<CurveKey>& GetKeys() const { return keys_; }
	/// <summary>エディタからの直接編集用。編集後は SortKeys() を呼ぶこと</summary>
	std::vector<CurveKey>& GetKeysForEdit() { return keys_; }
	/// <summary>キーを時刻順に並べ直す</summary>
	void SortKeys();

	/// <summary>from → to へ一直線に変化するカーブを作る</summary>
	static Curve Linear(float from, float to);
	/// <summary>常に一定値を返すカーブを作る</summary>
	static Curve Constant(float value);

private:
	// time の昇順で保持する
	std::vector<CurveKey> keys_;
};
