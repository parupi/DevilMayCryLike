#include "Fade.h"
#include "base/TextureManager.h"
#include <algorithm>
#include "imgui/imgui.h"
#include "base/Particle/ParticleManager.h"
#include <math/Easing.h>

void Fade::Initialize()
{
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("white.png");
	sprite_->SetSize(Vector2{ 1280.0f,720.0f });
	sprite_->SetTextureSize(Vector2{ 1280,720 });

	//particleManager_ = std::make_unique<ParticleManager>();
	//particleManager_->Initialize();
	//particleManager_->CreateParticleGroup("sceneChangeParticle", "resource/snow.png");
	//emitter_ = std::make_unique<ParticleEmitter>();
	//emitter_->Initialize(particleManager_.get(), "sceneChangeParticle");

	//particleManager_->Emit("sceneChangeParticle", { 0.0f, 10.0f, 0.0f }, 10);
}

void Fade::Update()
{
	//emitter_->Update({ 0.0f, 10.0f, 0.0f }, 64);
	//particleManager_->Update();

	switch (status_) {
	case Status::None:
		break;
	case Status::FadeIn:
		FadeInUpdate();
		break;
	case Status::FadeOut:
		FadeOutUpdate();
		break;
	}
}

void Fade::Draw()
{
	//ParticleResources::GetInstance()->DrawSet();
	//particleManager_->Draw();
}

void Fade::DrawSprite()
{
	if (status_ != Status::None) {
		sprite_->Draw();
	}
}

void Fade::Start(Status status, float duration)
{
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::Stop()
{
	status_ = Status::None;
}

bool Fade::IsFinished() const
{
	switch (status_) {
	case Status::FadeIn:
	case Status::FadeOut:
		if (counter_ >= duration_) {
			return true;
		}
		else {
			return false;
		}
	}
	return true;
}

void Fade::FadeOutUpdate()
{
	counter_ += DeltaTime::GetDeltaTime();
	if (counter_ >= duration_) {
		counter_ = duration_;
	}

	float progress = counter_ / duration_; // 進捗（0.0 ～ 1.0）
	//float easedAlpha = easeInQuint(progress);

	sprite_->SetColor(Vector4{ 1, 1, 1, progress });
	sprite_->Update();

	alphaCounter_ += DeltaTime::GetDeltaTime();
	alphaCounter_ = std::clamp(alphaCounter_, 0.0f, 1.0f);
	//float progressParticle = alphaCounter_ / 0.5f;
	alpha_ = easeOutCubic(progress);

	//particleManager_->SetAlpha("sceneChangeParticle", alphaCounter_);
}

void Fade::FadeInUpdate()
{
	counter_ += DeltaTime::GetDeltaTime();
	if (counter_ >= duration_) {
		counter_ = duration_;
	}

	float progress = counter_ / duration_; // 進捗（0.0 ～ 1.0）
	//float easedAlpha = easeOutQuint(progress);

	sprite_->SetColor(Vector4{ 1,1,1,1.0f - progress });
	sprite_->Update();

	alphaCounter_ -= DeltaTime::GetDeltaTime();
	alphaCounter_ = std::clamp(alphaCounter_, 0.0f, 1.0f);

	//particleManager_->SetAlpha("sceneChangeParticle", alphaCounter_);

}