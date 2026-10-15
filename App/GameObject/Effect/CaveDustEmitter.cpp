#include "CaveDustEmitter.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Stage/StageEnvironment.h"
#include "Utility/Logger.h"
#include "World3D/Object/Object3dManager.h"
#include "GameObject/Light/StageSpotLight.h"
#include "World3D/WorldTransform.h"
#include "Math/MathUtils.h"
#include <algorithm>
#include <cmath>

namespace {
// CaveDustMote の寿命の平均[秒]。先に撒いておく数の見積もりに使う（.vfx.json の min/maxLifeTime の中ほど）
constexpr float kAverageLifeTime = 8.0f;
// 一度に撒きすぎないための上限（グループの上限 2048 に余裕を残す）
constexpr int kMaxPrewarm = 400;
} // namespace

CaveDustEmitter::CaveDustEmitter()
	: rng_(std::random_device{}())
{
	ParticleManager& particles = ParticleManager::GetInstance();
	// パーティクルグループはシーンをまたいで残るので、未登録のときだけ読む
	if (!particles.GetParticleGroups().contains(kParticleName)) {
		if (!particles.LoadVFX(kVfxName)) {
			Logger::Log("CaveDustEmitter: Resource/VFX/CaveDust.vfx.json を読み込めませんでした（埃が出ません）");
		}
	}
	canEmit_ = particles.GetParticleGroups().contains(kParticleName);
	CollectSpotLights();
}

void CaveDustEmitter::CollectSpotLights()
{
	cones_.clear();
	for (Object3d* object : Object3dManager::GetInstance().GetAllObject()) {
		auto* spot = dynamic_cast<StageSpotLight*>(object);
		if (!spot || spot->GetParams().particleLighting <= 0.0f) continue;
		const StageSpotLight::Params& p = spot->GetParams();
		// 作った直後はワールド行列がまだ組まれていないので、位置はトランスフォームから、向きは回転から直接出す
		// （スポットライトは親を持たない前提）
		WorldTransform* t = spot->GetWorldTransform();
		Cone cone;
		cone.apex = t->GetTranslation();
		cone.axis = Normalize(RotateVector(Vector3{ 0.0f, -1.0f, 0.0f }, t->GetRotation()));
		cone.tanOuter = std::tan(std::clamp(p.outerAngle, 0.5f, 89.0f) * 3.14159265f / 180.0f);
		cone.length = p.distance;
		cones_.push_back(cone);
	}
}

void CaveDustEmitter::Update(float deltaTime, const Vector3& playerPosition, const Vector3& cameraPosition)
{
	const StageEnvironment::DustSettings& settings = StageEnvironment::GetDustSettings();
	if (!canEmit_ || !settings.enabled) {
		return;
	}

	// スポットライトの光の中の埃。最初の1回だけ寿命ぶんを先に撒く
	if (settings.conePerSecond > 0.0f && !cones_.empty()) {
		if (!conePrewarmed_) {
			conePrewarmed_ = true;
			for (const Cone& cone : cones_) {
				const int count = (std::min)(static_cast<int>(settings.conePerSecond * kAverageLifeTime), kMaxPrewarm);
				for (int i = 0; i < count; ++i) {
					EmitInCone(cone.apex, cone.axis, cone.tanOuter, cone.length);
				}
			}
		}
		coneTimer_ += std::clamp(deltaTime, 0.0f, 0.1f) * settings.conePerSecond;
		while (coneTimer_ >= 1.0f) {
			coneTimer_ -= 1.0f;
			for (const Cone& cone : cones_) {
				EmitInCone(cone.apex, cone.axis, cone.tanOuter, cone.length);
			}
		}
	}

	if (settings.perSecond <= 0.0f) {
		return;
	}
	// プレイヤーとカメラの間を中心にする（高さはプレイヤー基準）
	const Vector3 center{
		playerPosition.x + (cameraPosition.x - playerPosition.x) * settings.towardCamera,
		playerPosition.y,
		playerPosition.z + (cameraPosition.z - playerPosition.z) * settings.towardCamera };

	if (!prewarmed_) {
		prewarmed_ = true;
		const int count = (std::min)(static_cast<int>(settings.perSecond * kAverageLifeTime), kMaxPrewarm);
		for (int i = 0; i < count; ++i) {
			EmitOne(center);
		}
	}

	// 読み込み直後などの大きな飛びで一度に撒かないようにする
	timer_ += std::clamp(deltaTime, 0.0f, 0.1f) * settings.perSecond;
	while (timer_ >= 1.0f) {
		timer_ -= 1.0f;
		EmitOne(center);
	}
}

void CaveDustEmitter::EmitOne(const Vector3& center)
{
	const StageEnvironment::DustSettings& settings = StageEnvironment::GetDustSettings();
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	// 円の中に一様に散らす（半径に sqrt を掛けないと中心に寄る）
	const float r = settings.radius * std::sqrt(unit(rng_));
	const float a = unit(rng_) * 6.2831853f;
	const float y = settings.heightMin + (settings.heightMax - settings.heightMin) * unit(rng_);
	const Vector3 position{ center.x + std::cos(a) * r, center.y + y, center.z + std::sin(a) * r };
	ParticleManager::GetInstance().Emit(kParticleName, position, 1);
}

void CaveDustEmitter::EmitInCone(const Vector3& apex, const Vector3& axis, float tanOuter, float length)
{
	const StageEnvironment::DustSettings& settings = StageEnvironment::GetDustSettings();
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	// 光軸に沿った位置。光源のすぐ近く（霧の上）は見えないので、床側の範囲だけに撒く
	const float t = length * (settings.coneStart + (settings.coneEnd - settings.coneStart) * unit(rng_));
	// その位置での円の中に一様に散らす。外側の角度より少し内側に収めて、光の外へはみ出しにくくする
	const float r = tanOuter * t * 0.9f * std::sqrt(unit(rng_));
	const float a = unit(rng_) * 6.2831853f;
	// 光軸に垂直な2軸
	const Vector3 up = std::abs(axis.y) < 0.99f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
	Vector3 u{ up.y * axis.z - up.z * axis.y, up.z * axis.x - up.x * axis.z, up.x * axis.y - up.y * axis.x };
	const float ul = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
	u = { u.x / ul, u.y / ul, u.z / ul };
	const Vector3 v{ axis.y * u.z - axis.z * u.y, axis.z * u.x - axis.x * u.z, axis.x * u.y - axis.y * u.x };
	const float c = std::cos(a) * r;
	const float s = std::sin(a) * r;
	const Vector3 position{
		apex.x + axis.x * t + u.x * c + v.x * s,
		apex.y + axis.y * t + u.y * c + v.y * s,
		apex.z + axis.z * t + u.z * c + v.z * s };
	ParticleManager::GetInstance().Emit(kParticleName, position, 1);
}
