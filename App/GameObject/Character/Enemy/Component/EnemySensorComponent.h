#pragma once
#include <Math/Vector3.h>

class Enemy;

/// <summary>
/// プレイヤー感知コンポーネント。
/// 毎フレーム Update() を呼ぶと距離・発見フラグを更新する。
/// </summary>
class EnemySensorComponent {
public:
    void Update(Enemy& enemy);

    bool  IsPlayerDetected()      const { return detected_; }
    float GetDistanceToPlayer()   const { return distance_; }
    Vector3 GetDirectionToPlayer() const { return direction_; }

    void SetDetectionRange(float range) { detectionRange_ = range; }

private:
    float   detectionRange_ = 12.0f;
    bool    detected_       = false;
    float   distance_       = 0.0f;
    Vector3 direction_{};
};
