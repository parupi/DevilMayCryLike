#pragma once
#include "math/Vector3.h"
#include <random>

class HitStop
{
public:
    struct HitStopData {
        bool   isActive = false;
        Vector3 translate{};
        float progress = 0.0f; 
        float  timeScale = 1.0f; 
    };

    HitStop() = default;
    ~HitStop() = default;

    void Update(float deltaTime);
    void Start(float time, float intensity, float stopScale = 0.0f);

    HitStopData GetHitStopData() const { return hitStopData_; }
    bool IsActive() const { return hitStopData_.isActive; }

    float GetTimeScale() const { return hitStopData_.timeScale; }

private:
    HitStopData hitStopData_;
    float intensity_ = 0.0f;
    float maxTime_ = 0.0f;
    float timer_ = 0.0f;
    float stopScale_ = 0.0f;

    // 乱数生成器（毎フレームrand()せずにここで保持）
    std::mt19937 mt{ std::random_device{}() };
    std::uniform_real_distribution<float> dist{ -1.0f, 1.0f };
};
