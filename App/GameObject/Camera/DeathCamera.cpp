#include "DeathCamera.h"
#include "GameObject/Player/Player.h"
#include <random>
#include <math/Easing.h>

DeathCamera::DeathCamera(const std::string& cameraName, Camera* sourceCamera, Player* player)
    : Camera(cameraName), player_(player)
{
    // ゲームカメラの位置と回転を引き継ぎ
    GetTranslate() = sourceCamera->GetTranslate();
    GetRotate() = sourceCamera->GetRotate();

    basePos_ = GetTranslate();
    baseLookAt_ = player_->GetWorldTransform()->GetTranslation();

    // 乱数エンジン初期化
    randomEngine_ = std::mt19937(seedGenerator_());

    Camera::Update();
}

void DeathCamera::Update()
{
    if (!player_) return;

    float dt = DeltaTime::GetDeltaTime();
    zoomTime_ += dt;

    // === シェイク強度の減衰 ===
    const float maxShake = 0.2f;  // 最大揺れ幅
    const float minShake = 0.0f;  // 最小揺れ幅
    const float t = std::min(zoomTime_ / totalTime_, 1.0f);
    float shakeStrength = Lerp(minShake, maxShake, t); // 時間経過で揺れを強める

    // === ランダムシェイク ===
    std::uniform_real_distribution<float> shakeDist(-shakeStrength, shakeStrength);
    Vector3 shakeOffset{
        shakeDist(randomEngine_),
        shakeDist(randomEngine_) * 0.7f, // 縦揺れは少し弱めに
        shakeDist(randomEngine_) * 0.3f  // 前後方向は控えめに
    };

    // === ズームイン処理 ===
    const float zoomStart = 0.0f;
    const float zoomEnd = 8.0f;
    float zoomAmount = Lerp(zoomStart, zoomEnd, easeOutCubic(t));

    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
    Vector3 toPlayer = Normalize(playerPos - basePos_);

    // シェイク＋ズーム適用
    Vector3 cameraPos = basePos_ + toPlayer * zoomAmount + shakeOffset;

    // 注視点はプレイヤー
    GetTranslate() = cameraPos;
    LookAt(playerPos);

    Camera::Update();
}