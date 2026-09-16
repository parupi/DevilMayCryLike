#pragma once
#include "Math/Vector3.h"

class AnimationPlayer;

/// <summary>
/// 足音を鳴らすタイミングを決める部品。プレイヤーと骸骨で共用する。
///
/// 判定は2段構え:
///   1. 再生中のクリップに `footstep` イベントがあれば、それを使う（足が着く瞬間に正確に鳴る）
///   2. 無ければ**歩いた距離**で刻む
///
/// 距離で刻むのは、走りモーションの速度が移動速度に連動している限り歩幅と歩調が合うため。
/// アニメーション側にイベントを足せば自動的に 1. へ切り替わるので、後から精度を上げられる
/// （イベントは Animation ウィンドウか `Resource/models/&lt;モデル&gt;.anim.json` で足す）。
/// </summary>
class FootstepTracker
{
public:
	/// <summary>1歩ぶんの距離[m]。大きいほど足音の間隔が空く</summary>
	void SetStrideLength(float meters) { strideLength_ = (meters > 0.01f) ? meters : 0.01f; }

	/// <summary>
	/// 毎フレーム呼ぶ。足が着いたフレームだけ true を返す
	/// </summary>
	/// <param name="worldPosition">今の位置</param>
	/// <param name="grounded">接地しているか。浮いている間は刻まない</param>
	/// <param name="animation">null 可。イベントを持っていればそちらを優先する</param>
	bool Update(const Vector3& worldPosition, bool grounded, const AnimationPlayer* animation);

	/// <summary>
	/// 今の1歩が左右どちらか。左右で少しピッチを変えると、同じ音の繰り返しに聞こえにくくなる
	/// </summary>
	bool IsRightFoot() const { return (stepCount_ % 2) == 1; }

	/// <summary>位置が飛んだとき（ワープ・リスポーン）に呼ぶ。次のフレームは刻まない</summary>
	void Reset();

private:
	float strideLength_ = 1.9f;
	float travelled_ = 0.0f;
	int stepCount_ = 0;
	Vector3 previousPosition_{};
	bool hasPrevious_ = false;
};
