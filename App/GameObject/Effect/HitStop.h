#pragma once
#include "math/Vector3.h"
#include <random>

class HitStop
{
public:
    struct HitStopData {
        bool   isActive = false;
        Vector3 translate{};
        float progress = 0.0f;   // (追加) 経過割合 0.0-1.0
    };

    HitStop() = default;
    ~HitStop() = default;

    void Update();
    void Start(float time, float intensity);

    HitStopData GetHitStopData() const { return hitStopData_; }
    inline bool IsActive() const { return hitStopData_.isActive; }

private:
    HitStopData hitStopData_;
    float intensity_ = 0.0f;
    float maxTime_ = 0.0f;
    float timer_ = 0.0f;

    // 乱数生成器（毎フレームrand()せずにここで保持）
    std::mt19937 mt{ std::random_device{}() };
    std::uniform_real_distribution<float> dist{ -1.0f, 1.0f };
};
