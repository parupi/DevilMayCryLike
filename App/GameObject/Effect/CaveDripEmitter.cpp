#include "CaveDripEmitter.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Object/Object3d.h"
#include "Debugger/GlobalVariables.h"
#include "Audio/SoundManager.h"
#include "Audio/GameSoundLibrary.h"
#include "Stage/StageEnvironment.h"
#include "Utility/Logger.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kGravity = 9.8f; // CaveWaterDrop の Gravity と合わせること

bool StartsWith(const std::string& s, const char* prefix)
{
	return s.rfind(prefix, 0) == 0;
}
} // namespace

CaveDripEmitter::CaveDripEmitter()
	: rng_(std::random_device{}())
{
	ParticleManager& particles = ParticleManager::GetInstance();
	// パーティクルグループはシーンをまたいで残るので、未登録のときだけ読む
	if (!particles.GetParticleGroups().contains(kDropParticle)) {
		if (!particles.LoadVFX(kVfxName)) {
			Logger::Log("CaveDripEmitter: Resource/VFX/CaveDrip.vfx.json を読み込めませんでした（水滴が出ません）");
		}
	}
	const auto& groups = particles.GetParticleGroups();
	canEmit_ = groups.contains(kDropParticle) && groups.contains(kRippleParticle) && groups.contains(kSplashParticle);
	CollectSources();
}

void CaveDripEmitter::CollectSources()
{
	sources_.clear();
	for (Object3d* object : Object3dManager::GetInstance().GetAllObject()) {
		if (!object) continue;
		const std::string& model = object->GetModelName();
		const bool overhang = StartsWith(model, "Cave/CaveOverhang");
		const bool stalactite = StartsWith(model, "Cave/Stalactite");
		if (!overhang && !stalactite) continue;

		Source source;
		source.position = object->GetWorldTransform()->GetTranslation();
		// 部屋の内側＝部屋の中心（原点）の向き。壁沿いの岩は壁に垂直な軸へ寄せる（角の岩は斜めのまま）
		Vector3 inward{ -source.position.x, 0.0f, -source.position.z };
		const bool corner = std::abs(std::abs(source.position.x) - std::abs(source.position.z)) < 1.0f;
		if (!corner) {
			if (std::abs(inward.x) > std::abs(inward.z)) inward.z = 0.0f; else inward.x = 0.0f;
		}
		const float len = std::sqrt(inward.x * inward.x + inward.z * inward.z);
		source.inward = len > 1e-4f ? Vector3{ inward.x / len, 0.0f, inward.z / len } : Vector3{};
		source.isStalactite = stalactite;
		sources_.push_back(source);
	}
}

void CaveDripEmitter::Update(float deltaTime, const Vector3& center)
{
	const StageEnvironment::DripSettings& settings = StageEnvironment::GetDripSettings();
	const float dt = std::clamp(deltaTime, 0.0f, 0.1f);

	// 落ちている途中の水滴は、設定が切られても着地まで面倒を見る
	for (auto it = landings_.begin(); it != landings_.end();) {
		it->timeLeft -= dt;
		if (it->timeLeft > 0.0f) {
			++it;
			continue;
		}
		ParticleManager& particles = ParticleManager::GetInstance();
		particles.Emit(kRippleParticle, it->position, 1);
		particles.Emit(kSplashParticle, it->position, 4);
		SoundManager::GetInstance().PlaySE3D(GameSound::kWaterDrip, it->position, settings.volume);
		it = landings_.erase(it);
	}

	if (!canEmit_ || !settings.enabled || settings.perSecond <= 0.0f || sources_.empty()) {
		return;
	}

	// 落ちる高さから着地までの時間を出し、水滴の寿命をそれに揃える（床を突き抜けて落ち続けないように）
	const float fallHeight = (std::max)(settings.ceilingHeight - settings.floorHeight, 0.1f);
	const float fallTime = std::sqrt(2.0f * fallHeight / kGravity);
	if (std::abs(fallTime - appliedFallTime_) > 1e-4f) {
		GlobalVariables& gv = GlobalVariables::GetInstance();
		gv.SetValue(kDropParticle, "minLifeTime", fallTime);
		gv.SetValue(kDropParticle, "maxLifeTime", fallTime);
		appliedFallTime_ = fallTime;
	}

	spawnTimer_ += dt;
	if (spawnTimer_ < nextInterval_) {
		return;
	}
	spawnTimer_ = 0.0f;
	// 間隔はばらつかせる（等間隔だと機械仕掛けに聞こえる）。平均が 1/perSecond になる指数分布
	std::exponential_distribution<float> interval(settings.perSecond);
	nextInterval_ = std::clamp(interval(rng_), 0.15f, 6.0f);
	SpawnDrop(center);
}

void CaveDripEmitter::SpawnDrop(const Vector3& center)
{
	const StageEnvironment::DripSettings& settings = StageEnvironment::GetDripSettings();

	// プレイヤーの近くの岩だけを候補にする（遠くで鳴っても聞こえず、見えもしない）
	std::vector<const Source*> nearby;
	for (const Source& s : sources_) {
		const float dx = s.position.x - center.x;
		const float dz = s.position.z - center.z;
		if (dx * dx + dz * dz <= settings.radius * settings.radius) {
			nearby.push_back(&s);
		}
	}
	if (nearby.empty()) {
		return;
	}

	std::uniform_int_distribution<size_t> pick(0, nearby.size() - 1);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	const Source& source = *nearby[pick(rng_)];

	Vector3 xz = source.position;
	if (!source.isStalactite) {
		// 張り出しの下（壁から内側へ 1.5〜4.5m、壁に沿って ±3m）に落とす
		const float reach = 1.5f + 3.0f * unit(rng_);
		const float along = (unit(rng_) - 0.5f) * 6.0f;
		const Vector3 tangent{ -source.inward.z, 0.0f, source.inward.x };
		xz.x += source.inward.x * reach + tangent.x * along;
		xz.z += source.inward.z * reach + tangent.z * along;
	}

	const Vector3 top{ xz.x, settings.ceilingHeight, xz.z };
	ParticleManager::GetInstance().Emit(kDropParticle, top, 1);

	const float fallHeight = (std::max)(settings.ceilingHeight - settings.floorHeight, 0.1f);
	landings_.push_back({ Vector3{ xz.x, settings.floorHeight, xz.z }, std::sqrt(2.0f * fallHeight / kGravity) });
}
