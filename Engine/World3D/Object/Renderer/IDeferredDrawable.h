#pragma once
#include "Math/Vector3.h"

class IDeferredDrawable {
public:
	virtual void DrawGBuffer() = 0;
	/// <summary>
	/// カメラの視錐台カリング用の境界球（ワールド座標）。
	/// false を返したものはカリングせず常に描く（スキンモデルなど、形が変わって範囲を決められないもの）
	/// </summary>
	virtual bool GetCullingSphere([[maybe_unused]] Vector3& outCenter, [[maybe_unused]] float& outRadius) const { return false; }
	/// <summary>
	/// 同じモデルとまとめて描く側（StaticInstanceBatcher）へ回せたら true。
	/// true のときは DrawGBuffer を呼ばない（パスの最後にまとめて描かれる）
	/// </summary>
	virtual bool SubmitInstanced() { return false; }
	virtual ~IDeferredDrawable() = default;
};
