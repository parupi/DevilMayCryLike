#include "CaveDustEmitter.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Stage/StageEnvironment.h"
#include "Utility/Logger.h"
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
}

void CaveDustEmitter::Update(float deltaTime, const Vector3& center)
{
	const StageEnvironment::DustSettings& settings = StageEnvironment::GetDustSettings();
	if (!canEmit_ || !settings.enabled || settings.perSecond <= 0.0f) {
		return;
	}

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
