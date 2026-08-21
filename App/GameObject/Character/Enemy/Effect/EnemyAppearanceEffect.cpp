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
	BeginDeathStage(DeathStage::Launch);
	// ここではまだディゾルブを掛けない。
	// 掛けてしまうと「やられた瞬間から消え始める」ように見えるため、Dissolve 段階まで実体のままにする
}

void EnemyAppearanceEffect::BeginDeathStage(DeathStage stage) {
	deathStage_ = stage;
	stageTimer_ = 0.0f;
}

int EnemyAppearanceEffect::ConsumeEmitTicks(float deltaTime) {
	emitTimer_ += deltaTime;
	int ticks = 0;
	while (emitTimer_ >= kEmitInterval) {
		emitTimer_ -= kEmitInterval;
		++ticks;
	}
	return ticks;
}

void EnemyAppearanceEffect::Update(float deltaTime) {
	switch (phase_) {
	case Phase::Appearing: {
		timer_ += deltaTime;
		float t = timer_ / appearDuration_;

		if (t >= 1.0f) {
			// 実体化完了。ディゾルブ上書きを解除して通常描画に戻す
			ClearDissolve();
			phase_ = Phase::None;
			break;
		}

		// 収束する粒子を一定間隔で発生させる
		const int ticks = ConsumeEmitTicks(deltaTime);
		if (ticks > 0) {
			EmitParticle(kSpawnGroup, appearEmitCount_ * ticks);
		}

		// ディゾルブイン（1 → 0 で実体化）
		ApplyDissolve(1.0f - t, kAppearEdgeColor);
		break;
	}
	case Phase::Dying:
		UpdateDeath(deltaTime);
		break;
	default:
		break;
	}
}

// 死亡演出は「吹き飛び → 死亡モーション → 黒いもや → ディゾルブ」の4段階。
// 段階が変わるのは時間だけで、動き（吹き飛びの慣性・重力・着地）は所有者側が見る
void EnemyAppearanceEffect::UpdateDeath(float deltaTime) {
	stageTimer_ += deltaTime;
	// 段階をまたいでも発生間隔が崩れないよう、毎フレーム進めておく
	const int ticks = ConsumeEmitTicks(deltaTime);

	switch (deathStage_) {
	case DeathStage::Launch:
		// 吹き飛んでいる間は何も出さない。倒したことをまず見せる
		if (stageTimer_ >= kDeathLaunchDuration) {
			BeginDeathStage(DeathStage::Motion);
		}
		break;

	case DeathStage::Motion:
		// 死亡モーションを見せる。終わり際からもやが漏れ始めて次の段階へ繋ぐ
		if (ticks > 0 && stageTimer_ >= kDeathMotionDuration - kSmokeLeadTime) {
			EmitParticle(kSmokeGroup, kSmokeLeadCount * ticks);
		}
		if (stageTimer_ >= kDeathMotionDuration) {
			BeginDeathStage(DeathStage::Smoke);
			// もやを一気に噴き出して「消える前触れ」を作る
			EmitParticle(kSmokeGroup, kSmokeBurstCount);
		}
		break;

	case DeathStage::Smoke:
		// 黒いもやが体を包む
		if (ticks > 0) {
			EmitParticle(kSmokeGroup, kSmokeEmitCount * ticks);
		}
		if (stageTimer_ >= kDeathSmokeDuration) {
			BeginDeathStage(DeathStage::Dissolve);
			// ここで初めてディゾルブ上書きを有効にする（0 = まだ全部残っている）
			ApplyDissolve(0.0f, kDeathEdgeColor);
		}
		break;

	case DeathStage::Dissolve: {
		const float t = stageTimer_ / kDeathDissolveDuration;

		if (t >= 1.0f) {
			// 完全に消えた状態で維持し、所有者に終了を通知する
			ApplyDissolve(1.0f, kDeathEdgeColor);
			phase_ = Phase::DeathFinished;
			break;
		}

		// 溶けた分がそのまま黒い粒子になって散っていくように見せる
		if (ticks > 0) {
			EmitParticle(kScatterGroup, kDeathEmitCount * ticks);
			EmitParticle(kSmokeGroup, kDissolveSmokeCount * ticks);
		}

		// ディゾルブアウト（0 → 1 で消滅）
		ApplyDissolve(t, kDeathEdgeColor);
		break;
	}
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
