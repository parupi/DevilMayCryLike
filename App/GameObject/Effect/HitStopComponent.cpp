#include "HitStopComponent.h"
#include <utility>

void HitStopComponent::Update(float sceneDt)
{
    if (timer_ > 0.0f) {
        timer_ -= sceneDt;
        localScale_ = stopScale_;  // 通常は0
    } else {
        localScale_ = 1.0f;
    }
}

void HitStopComponent::Apply(float duration, float scale)
{
    // 長い方を優先（上書き問題回避）
    timer_ = std::max(timer_, duration);
    stopScale_ = scale;
}
