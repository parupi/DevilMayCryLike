#include "VignetteExpandTransition.h"
#include <offscreen/VignetteEffect.h>
#include <offscreen/OffScreenManager.h>

VignetteExpandTransition::VignetteExpandTransition(const std::string& transitionName)
{
	name = transitionName;
}

void VignetteExpandTransition::Initialize()
{
    // 遷移で使うVignetteを用意
    std::unique_ptr<VignetteEffect> vignette = std::make_unique<VignetteEffect>("vignette");
    vignette->SetActive(true);
    vignette->GetEffectData().intensity = 1.0f;
    vignette->GetEffectData().radius = 1.0f;
    vignette->GetEffectData().softness = 2.0f;

    OffScreenManager::GetInstance()->AddEffect(std::move(vignette));
}

void VignetteExpandTransition::Start(bool isFadeOut)
{
	isFadeOut_ = isFadeOut;
	finished_ = false;

	// softness の初期値を設定
	// フェードアウトなら 2.0f → 0.0f、フェードインなら 0.0f → 2.0f に変化
	currentSoftness_ = isFadeOut ? 2.0f : 0.0f;
    static_cast<VignetteEffect*> (OffScreenManager::GetInstance()->FindEffect("vignette"))->GetEffectData().softness = currentSoftness_;
}

void VignetteExpandTransition::Update()
{
    const float speed = 0.05f; // 変化速度

    if (isFadeOut_) {
        // 2.0 → 0.0 に減少（暗転していく）
        currentSoftness_ -= speed;
        if (currentSoftness_ <= 0.0f) {
            currentSoftness_ = 0.0f;
            finished_ = true;
        }
    } else {
        // 0.0 → 2.0 に増加（明るくなっていく）
        currentSoftness_ += speed;
        if (currentSoftness_ >= 2.0f) {
            currentSoftness_ = 2.0f;
            finished_ = true;
        }
    }

    static_cast<VignetteEffect*> (OffScreenManager::GetInstance()->FindEffect("vignette"))->GetEffectData().softness = currentSoftness_;
}

void VignetteExpandTransition::Draw()
{
    // VignetteはOffScreenManagerで描画されるので、ここでは何もしない
}
