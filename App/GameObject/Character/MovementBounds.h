#pragma once
#include <Math/Vector3.h>

/// <summary>
/// キャラクターの移動可能範囲。鉛直軸まわりに回転した箱で表す。
///
/// 強制戦闘エリアは床の向きに合わせて斜めに置かれることがあるため、
/// 軸平行のmin/maxではなく「中心＋水平2軸＋各軸の半径」で持つ。
/// 制限をかけるのは水平方向だけで、Y（落下・ジャンプ）は制限しない。
/// minY/maxY は範囲を可視化する側が箱の高さを知るために持っているだけで、
/// 移動の制限には使わない。
/// </summary>
struct MovementBounds {
	Vector3 center{};                  // 箱の中心（ワールド座標）
	Vector3 axisU{ 1.0f, 0.0f, 0.0f }; // 水平面内の基準軸（単位ベクトル）
	Vector3 axisV{ 0.0f, 0.0f, 1.0f }; // axisU と直交する水平軸（単位ベクトル）
	float halfU = 0.0f;                // axisU 方向の半径
	float halfV = 0.0f;                // axisV 方向の半径
	float minY = 0.0f;                 // 箱の下端
	float maxY = 0.0f;                 // 箱の上端

	/// <summary>
	/// pos を範囲内へ押し戻す。axisU/axisV は水平なので Y は変化しない。
	/// </summary>
	/// <param name="outInwardNormal">押し戻した向き（内向きの単位ベクトル）</param>
	/// <returns>押し戻した場合 true</returns>
	bool ClampPosition(Vector3& pos, Vector3& outInwardNormal) const {
		const Vector3 d = pos - center;
		const float u = Dot(d, axisU);
		const float v = Dot(d, axisV);

		Vector3 push{};
		if (u < -halfU)     push += axisU * (-halfU - u);
		else if (u > halfU) push += axisU * (halfU - u);

		if (v < -halfV)     push += axisV * (-halfV - v);
		else if (v > halfV) push += axisV * (halfV - v);

		if (Length(push) <= 0.0f) {
			outInwardNormal = {};
			return false;
		}

		pos += push;
		outInwardNormal = Normalize(push);
		return true;
	}

	/// <summary>各軸の半径を margin だけ内側に縮めた範囲を返す（負にはならない）</summary>
	MovementBounds Shrunk(float margin) const {
		MovementBounds result = *this;
		result.halfU = (halfU > margin) ? halfU - margin : 0.0f;
		result.halfV = (halfV > margin) ? halfV - margin : 0.0f;
		return result;
	}
};
