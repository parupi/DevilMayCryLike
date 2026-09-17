#include "DissolveOutEffect.h"

#include <Graphics/Rendering/Particle/ParticleManager.h>
#include <World3D/Object/Renderer/BaseRenderer.h>

#include <algorithm>

void DissolveOutEffect::AddRenderer(BaseRenderer* renderer) {
	if (!renderer) return;

	// 二重登録すると同じレンダラーへ何度も書き込むだけなので弾いておく
	const auto it = std::find(renderers_.begin(), renderers_.end(), renderer);
	if (it != renderers_.end()) return;

	renderers_.push_back(renderer);
}

void DissolveOutEffect::Start() {
	isPlaying_ = true;
	isFinished_ = false;
	timer_ = 0.0f;
	emitTimer_ = 0.0f;
	// 0 = まだ全部残っている状態。ここで上書きを有効にする
	Apply(0.0f);
}

void DissolveOutEffect::Update(float deltaTime, const Vector3& position) {
	if (!isPlaying_) return;

	if (timer_ <= 0.0f) {
		// 溶け始めに一気に撒いて、消える前触れを作る
		Emit(kBurstCount, position);
	}

	timer_ += deltaTime;
	const float t = GetProgress();

	// 溶けた分がそのまま黒い粒子になって散っていくように見せる
	emitTimer_ += deltaTime;
	while (emitTimer_ >= kEmitInterval) {
		emitTimer_ -= kEmitInterval;
		Emit(kEmitCount, position);
	}

	Apply(t);

	if (t >= 1.0f) {
		isPlaying_ = false;
		isFinished_ = true;
	}
}

void DissolveOutEffect::Reset() {
	isPlaying_ = false;
	isFinished_ = false;
	timer_ = 0.0f;
	emitTimer_ = 0.0f;
	for (BaseRenderer* renderer : renderers_) {
		if (renderer) renderer->SetDissolveThreshold(-1.0f); // -1 = 上書きしない
	}
}

float DissolveOutEffect::GetProgress() const {
	if (duration_ <= 0.0f) return 1.0f;
	return std::clamp(timer_ / duration_, 0.0f, 1.0f);
}

void DissolveOutEffect::Apply(float threshold) {
	for (BaseRenderer* renderer : renderers_) {
		if (!renderer) continue;
		renderer->SetDissolveThreshold(threshold);
		renderer->SetDissolveEdgeWidth(kEdgeWidth);
		renderer->SetDissolveEdgeColor(edgeColor_);
	}
}

void DissolveOutEffect::Emit(int count, const Vector3& position) {
	if (count <= 0) return;

	// グループが登録されていないシーンでは何もしない
	auto& groups = ParticleManager::GetInstance().GetParticleGroups();
	if (!groups.contains(kSmokeGroup)) return;

	ParticleManager::GetInstance().Emit(kSmokeGroup, position, static_cast<uint32_t>(count));
}
