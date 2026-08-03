#define NOMINMAX
#include "ParticleMath.h"
#include "Math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace ParticleMath {

	void BuildBasis(const Vector3& axis, Vector3& outRight, Vector3& outUp)
	{
		// axis と平行になりにくい補助ベクトルを選ぶ（平行だと外積が潰れる）
		const Vector3 helper = (std::fabs(axis.y) > 0.99f)
			? Vector3{ 0.0f, 0.0f, 1.0f }
			: Vector3{ 0.0f, 1.0f, 0.0f };

		outRight = Normalize(Cross(helper, axis));
		outUp = Cross(axis, outRight);
	}

	Vector3 RandomDirectionInCone(const Vector3& axis, float spreadDegrees, std::mt19937& rng)
	{
		constexpr float kPi = std::numbers::pi_v<float>;

		const float halfAngle = std::clamp(spreadDegrees, 0.0f, 180.0f) * (kPi / 180.0f);
		if (halfAngle <= 0.0f) {
			return axis;
		}

		std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

		// cosθ を一様にサンプリングすると球冠上で一様になる。
		// θ を一様に取ると軸の周りに粒が密集して見えてしまう
		const float cosMax = std::cos(halfAngle);
		const float cosTheta = 1.0f - dist01(rng) * (1.0f - cosMax);
		const float sinTheta = std::sqrt((std::max)(0.0f, 1.0f - cosTheta * cosTheta));
		const float phi = dist01(rng) * 2.0f * kPi;

		Vector3 right{}, up{};
		BuildBasis(axis, right, up);

		return right * (sinTheta * std::cos(phi))
			+ up * (sinTheta * std::sin(phi))
			+ axis * cosTheta;
	}

	namespace {
		/// 行ベクトル規約で、各行がローカル軸の変換先になる行列を組む。
		/// MakeDirectionMatrix と同じ約束（rowZ == Cross(rowX, rowY)）で揃えてある
		Matrix4x4 MakeBasisMatrix(const Vector3& rowX, const Vector3& rowY, const Vector3& rowZ)
		{
			Matrix4x4 m = MakeIdentity4x4();
			m.m[0][0] = rowX.x; m.m[0][1] = rowX.y; m.m[0][2] = rowX.z;
			m.m[1][0] = rowY.x; m.m[1][1] = rowY.y; m.m[1][2] = rowY.z;
			m.m[2][0] = rowZ.x; m.m[2][1] = rowZ.y; m.m[2][2] = rowZ.z;
			return m;
		}
	} // namespace

	Matrix4x4 MakeDirectionMatrix(const Vector3& dir, bool baseAxisIsY)
	{
		const float length = Length(dir);
		if (length < 0.0001f) {
			return MakeIdentity4x4();
		}
		const Vector3 f = dir / length;

		Vector3 right{}, up{};
		BuildBasis(f, right, up);

		Vector3 rowX{}, rowY{}, rowZ{};
		if (baseAxisIsY) {
			// ローカル +Y を f へ。(right, f, right×f) が右手系になる
			rowX = right;
			rowY = f;
			rowZ = Cross(right, f);
		} else {
			// ローカル +Z を f へ。BuildBasis の up は f×right なのでそのまま使える
			rowX = right;
			rowY = up;
			rowZ = f;
		}

		return MakeBasisMatrix(rowX, rowY, rowZ);
	}

	Matrix4x4 MakeAxisBillboardMatrix(const Vector3& toCamera, const Vector3& axis)
	{
		// カメラ方向から軸成分を抜く。残りが「軸まわりに回った結果の正面」になる
		Vector3 forward = toCamera - axis * Dot(toCamera, axis);

		const float length = Length(forward);
		if (length < 0.0001f) {
			// カメラが軸の真上・真下にいるケース。この向きからは板が線にしか見えないので回さない
			return MakeIdentity4x4();
		}
		forward = forward / length;

		// rowZ == Cross(rowX, rowY) を満たすよう rowX を決める
		return MakeBasisMatrix(Cross(axis, forward), axis, forward);
	}

	bool TryMakeVelocityBillboardMatrix(const Vector3& velocity, const Vector3& toCamera, Matrix4x4& outMatrix)
	{
		const float speed = Length(velocity);
		if (speed < 0.0001f) {
			// 止まっている粒には「進行方向」が無い
			return false;
		}
		const Vector3 v = velocity / speed;

		const float toCameraLength = Length(toCamera);
		if (toCameraLength < 0.0001f) {
			return false;
		}
		const Vector3 toCam = toCamera / toCameraLength;

		// 速度とカメラ方向の両方に直交する軸。真っ直ぐこちらへ飛んでくる粒では潰れる
		Vector3 right = Cross(v, toCam);
		const float rightLength = Length(right);
		if (rightLength < 0.0001f) {
			return false;
		}
		right = right / rightLength;

		// ローカル +Y を速度へ。ローカル +Z は自動的にカメラ側を向く
		outMatrix = MakeBasisMatrix(right, v, Cross(right, v));
		return true;
	}

} // namespace ParticleMath
