#define NOMINMAX
#include "ParticleRenderSystem.h"
#include "ParticleMath.h"
#include "World3D/Camera/BaseCamera.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

	/// <summary>
	/// その形状メッシュの基準軸が +Y かどうか。
	/// MeshGenerator の Ring / Cylinder はXZ平面基準（法線 +Y）、Plane はXY平面（法線 +Z）で作られている。
	/// </summary>
	bool IsShapeBaseAxisY(PrimitiveType shape)
	{
		return shape == PrimitiveType::Ring || shape == PrimitiveType::Cylinder;
	}

	/// <summary>
	/// スプライトシートの現在のコマを UV の (オフセットxy, スケールzw) として返す。
	/// コマ分割が 1x1 のときは (0,0,1,1) を返すので、アニメ未使用のグループはテクスチャ全体を使う。
	/// </summary>
	Vector4 CalcUvFrame(const Particle& particle, const ParticleParameters& params)
	{
		const int columns = (std::max)(params.animColumns, 1);
		const int rows = (std::max)(params.animRows, 1);
		const int frameCount = columns * rows;

		if (frameCount <= 1) {
			return Vector4{ 0.0f, 0.0f, 1.0f, 1.0f };
		}

		// AnimFps が正なら実時間でコマを送る。0以下なら寿命いっぱいでシートを1周させる
		// （爆発のように「発生から消滅までで1周」させたい場合はこちらが便利）
		float frame = (params.animFps > 0.0f)
			? particle.currentTime * params.animFps
			: ((particle.lifeTime > 0.0f) ? (particle.currentTime / particle.lifeTime) : 1.0f)
				* static_cast<float>(frameCount);

		if (params.animLoop) {
			// int にしてから剰余を取ると、長寿命 × 高Fps で桁が溢れるのでfloatのうちに畳む
			frame = std::fmod(frame, static_cast<float>(frameCount));
			if (frame < 0.0f) frame += static_cast<float>(frameCount);
		}

		int frameIndex = static_cast<int>(frame);
		frameIndex = std::clamp(frameIndex, 0, frameCount - 1); // 非ループ時は最後のコマで止まる

		const float invColumns = 1.0f / static_cast<float>(columns);
		const float invRows = 1.0f / static_cast<float>(rows);

		// コマ順は左上から右へ、行が尽きたら次の段へ
		return Vector4{
			static_cast<float>(frameIndex % columns) * invColumns,
			static_cast<float>(frameIndex / columns) * invRows,
			invColumns,
			invRows
		};
	}

} // namespace

void ParticleRenderSystem::BuildInstances(const ParticleGroup& group, BaseCamera* camera, std::vector<InstanceData>& outInstances)
{
	const std::vector<Particle>& particles = group.particles;

	outInstances.clear();
	outInstances.reserve(particles.size());

	// 行列はカメラが毎フレーム計算済みのものをそのまま使う。
	// ここで再計算するとFOV変化（攻撃ズーム）やアスペクト比・farクリップと食い違い、
	// パーティクルだけが別のカメラで描かれてしまう。
	const Matrix4x4& cameraMatrix = camera->GetWorldMatrix();
	const Matrix4x4& viewProjectionMatrix = camera->GetViewProjectionMatrix();

	// 行ベクトル規約なので、ワールド行列の4行目がカメラのワールド座標
	const Vector3 cameraPosition{ cameraMatrix.m[3][0], cameraMatrix.m[3][1], cameraMatrix.m[3][2] };

	Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
	Matrix4x4 screenBillboardMatrix = backToFrontMatrix * cameraMatrix;
	screenBillboardMatrix.m[3][0] = 0.0f;
	screenBillboardMatrix.m[3][1] = 0.0f;
	screenBillboardMatrix.m[3][2] = 0.0f;

	// ビルボードの種類とアニメ設定はグループ共通なので、ループの外で1回だけ読む
	const ParticleParameters& params = group.params;
	const auto billboardType = static_cast<BillboardType>(params.billboardType);
	const bool shapeBaseAxisY = IsShapeBaseAxisY(group.shape);

	for (const auto& p : particles)
	{
		Vector3 scale = p.transform.scale;
		Matrix4x4 translateMatrix = MakeTranslateMatrix(p.transform.translate);
		Matrix4x4 worldMatrix{};

		if (p.isBillboard) {
			// 既定は従来どおりのスクリーンビルボード。種類が指定されていればそれで置き換える
			Matrix4x4 orientMatrix = screenBillboardMatrix;

			switch (billboardType) {
			case BillboardType::AxisY:
				orientMatrix = ParticleMath::MakeAxisBillboardMatrix(
					cameraPosition - p.transform.translate, Vector3{ 0.0f, 1.0f, 0.0f });
				break;

			case BillboardType::Velocity: {
				Matrix4x4 velocityMatrix{};
				// 止まっている粒やカメラへ真っ直ぐ飛んでくる粒は向きが決まらないので、
				// その場合だけスクリーンビルボードのまま描く
				if (ParticleMath::TryMakeVelocityBillboardMatrix(
						p.velocity, cameraPosition - p.transform.translate, velocityMatrix)) {
					orientMatrix = velocityMatrix;
					// ローカル+Yが進行方向なので、Yを伸ばすと尾を引いた火花になる
					scale.y *= 1.0f + Length(p.velocity) * params.velocityStretch;
				}
				break;
			}

			case BillboardType::Screen:
			default:
				break;
			}

			worldMatrix = MakeScaleMatrix(scale) * orientMatrix * translateMatrix;
		} else if (p.orientToDirection) {
			// 発生時に渡された方向へ形状を向ける（インパクトリング・斬撃板など）
			const Matrix4x4 orientMatrix =
				ParticleMath::MakeDirectionMatrix(p.orientDir, shapeBaseAxisY);
			worldMatrix = MakeScaleMatrix(scale) * orientMatrix * translateMatrix;
		} else {
			worldMatrix = MakeAffineMatrix(scale, p.transform.rotate, p.transform.translate);
		}

		InstanceData id{};
		id.world = worldMatrix;
		id.wvp = worldMatrix * viewProjectionMatrix;
		id.color = p.color;
		id.uvOffsetScale = CalcUvFrame(p, params);

		outInstances.push_back(id);
	}
}
