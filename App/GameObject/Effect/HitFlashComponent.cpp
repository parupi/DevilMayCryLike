#include "HitFlashComponent.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include <algorithm>

HitFlashComponent::HitFlashComponent()
{
	// 頭が強く、すぐ落ちる形。直線的に減らすと「ぼんやり光った」ように見える
	decayCurve_.SetKeys({
		{ 0.0f, 1.0f },
		{ 0.25f, 0.45f },
		{ 1.0f, 0.0f },
		});
}

void HitFlashComponent::AddRenderer(BaseRenderer* renderer)
{
	if (!renderer) return;

	// 同じレンダラーを二重に登録すると savedTint が自分の発光で上書きされてしまう
	const auto it = std::find_if(targets_.begin(), targets_.end(),
		[renderer](const Target& target) { return target.renderer == renderer; });
	if (it != targets_.end()) return;

	targets_.push_back(Target{ renderer, Vector4{} });
}

void HitFlashComponent::Start(float duration, float intensity)
{
	if (duration <= 0.0f) return;

	// 再生中の再ヒットでは元ティントを取り直さない（自分の白を保存してしまうため）
	if (!isPlaying_) {
		for (Target& target : targets_) {
			if (target.renderer) {
				target.savedTint = target.renderer->GetEmissiveTint();
			}
		}
	}

	// 連続ヒット中は「より強く・より長く」を優先する
	duration_ = isPlaying_ ? (std::max)(duration_, duration) : duration;
	intensity_ = isPlaying_ ? (std::max)(intensity_, intensity) : intensity;
	timer_ = 0.0f;
	isPlaying_ = true;
}

void HitFlashComponent::Update(float deltaTime)
{
	if (!isPlaying_) return;

	timer_ += deltaTime;

	if (timer_ >= duration_) {
		RestoreAll();
		isPlaying_ = false;
		return;
	}

	const float alpha = intensity_ * decayCurve_.Evaluate(timer_ / duration_);

	for (Target& target : targets_) {
		if (!target.renderer) continue;

		// 相手（アーマー発光など）の方が強く光っているフレームは譲る
		if (alpha <= target.renderer->GetEmissiveTint().w) continue;

		target.renderer->SetEmissiveTint(Vector4{ kFlashR, kFlashG, kFlashB, alpha });
	}
}

void HitFlashComponent::Stop()
{
	if (!isPlaying_) return;
	RestoreAll();
	isPlaying_ = false;
}

void HitFlashComponent::RestoreAll()
{
	for (Target& target : targets_) {
		if (target.renderer) {
			target.renderer->SetEmissiveTint(target.savedTint);
		}
	}
}
