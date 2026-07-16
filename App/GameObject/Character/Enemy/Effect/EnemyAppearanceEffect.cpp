#include "EnemyAppearanceEffect.h"
#include <World3D/Object/Object3d.h>
#include <World3D/Object/Renderer/BaseRenderer.h>
#include "Graphics/Rendering/Particle/ParticleManager.h"

void EnemyAppearanceEffect::Initialize(Object3d* owner) {
	owner_ = owner;
}

void EnemyAppearanceEffect::AddRenderer(BaseRenderer* renderer) {
	if (renderer) {
		renderers_.push_back(renderer);
	}
}

void EnemyAppearanceEffect::StartAppear() {
	phase_ = Phase::Appearing;
	timer_ = 0.0f;
	emitTimer_ = 0.0f;
	// 最初のフレームから完全に溶けた状態で描画されるように即適用する
	ApplyDissolve(1.0f, kAppearEdgeColor);
}

void EnemyAppearanceEffect::StartDeath() {
	phase_ = Phase::Dying;
	timer_ = 0.0f;
	emitTimer_ = 0.0f;
	ApplyDissolve(0.0f, kDeathEdgeColor);
	// 死亡の瞬間に粒子を一気に撒き散らす
	EmitParticle("EnemyDeathParticle", kDeathBurstCount);
}

void EnemyAppearanceEffect::Update(float deltaTime) {
	switch (phase_) {
	case Phase::Appearing: {
		timer_ += deltaTime;
		float t = timer_ / kAppearDuration;

		if (t >= 1.0f) {
			// 実体化完了。ディゾルブ上書きを解除して通常描画に戻す
			ClearDissolve();
			phase_ = Phase::None;
			break;
		}

		// 収束する粒子を一定間隔で発生させる
		emitTimer_ += deltaTime;
		while (emitTimer_ >= kEmitInterval) {
			EmitParticle("EnemySpawnParticle", kAppearEmitCount);
			emitTimer_ -= kEmitInterval;
		}

		// ディゾルブイン（1 → 0 で実体化）
		ApplyDissolve(1.0f - t, kAppearEdgeColor);
		break;
	}
	case Phase::Dying: {
		timer_ += deltaTime;
		float t = timer_ / kDeathDuration;

		if (t >= 1.0f) {
			// 完全に消えた状態で維持し、所有者に終了を通知する
			ApplyDissolve(1.0f, kDeathEdgeColor);
			phase_ = Phase::DeathFinished;
			break;
		}

		// 拡散する粒子を一定間隔で発生させる
		emitTimer_ += deltaTime;
		while (emitTimer_ >= kEmitInterval) {
			EmitParticle("EnemyDeathParticle", kDeathEmitCount);
			emitTimer_ -= kEmitInterval;
		}

		// ディゾルブアウト（0 → 1 で消滅）
		ApplyDissolve(t, kDeathEdgeColor);
		break;
	}
	default:
		break;
	}
}

void EnemyAppearanceEffect::ApplyDissolve(float threshold, const Vector4& edgeColor) {
	for (auto* renderer : renderers_) {
		renderer->SetDissolveThreshold(threshold);
		renderer->SetDissolveEdgeWidth(kEdgeWidth);
		renderer->SetDissolveEdgeColor(edgeColor);
	}
}

void EnemyAppearanceEffect::ClearDissolve() {
	for (auto* renderer : renderers_) {
		renderer->SetDissolveThreshold(-1.0f);
	}
}

void EnemyAppearanceEffect::EmitParticle(const char* groupName, int count) {
	if (!owner_ || count <= 0) return;

	// グループが登録されていないシーンでは何もしない
	auto& groups = ParticleManager::GetInstance().GetParticleGroups();
	if (!groups.contains(groupName)) return;

	Vector3 position = owner_->GetWorldTransform()->GetWorldPos();
	ParticleManager::GetInstance().Emit(groupName, position, static_cast<uint32_t>(count));
}
