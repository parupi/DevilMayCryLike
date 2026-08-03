#pragma once
#include <random>
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"

/// <summary>
/// 方向付き発生・向き付けで使う幾何計算。
/// 発生側(ParticleManager)と描画側(ParticleRenderSystem)の両方から使うため、
/// どちらにも依存しない純粋な計算としてここに置いている。
/// </summary>
namespace ParticleMath {

	/// <summary>
	/// axis と直交する正規直交基底を作る。
	/// axis は正規化済みであること。
	/// </summary>
	void BuildBasis(const Vector3& axis, Vector3& outRight, Vector3& outUp);

	/// <summary>
	/// axis を軸とした半角 spreadDegrees のコーン内で、ランダムな単位ベクトルを返す。
	/// 球冠上で一様になるようサンプリングするので、軸の周りに粒が偏らない。
	/// </summary>
	/// <param name="axis">コーンの軸（正規化済みであること）</param>
	/// <param name="spreadDegrees">コーンの半角[度]。0=軸そのもの / 180=全方向</param>
	Vector3 RandomDirectionInCone(const Vector3& axis, float spreadDegrees, std::mt19937& rng);

	/// <summary>
	/// メッシュの基準軸を dir に向ける回転行列を作る。
	///
	/// オイラー角を経由すると MakeRotateXYZMatrix の合成順(Ry*Rx*Rz)に依存して
	/// 逆算が煩雑になるので、正規直交基底を直接組み立てている。
	/// 行ベクトル規約なので、行列の各行がローカル軸の変換先になる。
	/// </summary>
	/// <param name="dir">向けたい方向（正規化されていなくてもよい）</param>
	/// <param name="baseAxisIsY">
	/// true  = メッシュの基準軸が +Y（MeshGenerator の Ring / Cylinder）
	/// false = メッシュの基準軸が +Z（Plane）
	/// </param>
	Matrix4x4 MakeDirectionMatrix(const Vector3& dir, bool baseAxisIsY);

	/// <summary>
	/// 指定した軸まわりだけ回ってカメラを向くビルボード行列を作る（BillboardType::AxisY）。
	/// メッシュのローカル +Y が axis に、ローカル +Z がカメラ方向（axis と直交する成分）に写る。
	/// 炎や光の柱のように「立っている」ものが傾かないようにするために使う。
	/// </summary>
	/// <param name="toCamera">パーティクル → カメラ のベクトル（正規化不要）</param>
	/// <param name="axis">回転軸。通常はワールド +Y（正規化済みであること）</param>
	/// <returns>カメラが軸の真上にいるなど向きが決まらない場合は単位行列</returns>
	Matrix4x4 MakeAxisBillboardMatrix(const Vector3& toCamera, const Vector3& axis);

	/// <summary>
	/// 進行方向に軸を合わせつつ、面をカメラへ向けるビルボード行列を作る（BillboardType::Velocity）。
	/// メッシュのローカル +Y が velocity に写るので、ローカルYを引き伸ばすと尾を引いた火花になる。
	/// </summary>
	/// <param name="velocity">パーティクルの速度</param>
	/// <param name="toCamera">パーティクル → カメラ のベクトル（正規化不要）</param>
	/// <param name="outMatrix">成功時のみ書き込まれる</param>
	/// <returns>
	/// false = 速度がほぼ0、または速度がカメラ方向と平行で向きが決まらない。
	/// 呼び出し側は通常のスクリーンビルボードへ落とすこと
	/// </returns>
	bool TryMakeVelocityBillboardMatrix(const Vector3& velocity, const Vector3& toCamera, Matrix4x4& outMatrix);

} // namespace ParticleMath
