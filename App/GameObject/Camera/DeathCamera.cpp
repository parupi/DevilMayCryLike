#include "DeathCamera.h"
#include "GameObject/Character/Player/Player.h"
#include <random>
#include <algorithm>
#include <Math/Easing.h>
#include <Utility/DeltaTime.h>

DeathCamera::DeathCamera(const std::string& cameraName, BaseCamera* sourceCamera, Player* player)
    : BaseCamera(cameraName), player_(player)
{
    // ゲームカメラの位置と回転を引き継ぎ
    GetTranslate() = sourceCamera->GetTranslate();
    GetRotate() = sourceCamera->GetRotate();

    basePos_ = GetTranslate();

    // 乱数エンジン初期化
    randomEngine_ = std::mt19937(seedGenerator_());

    BaseCamera::Update();
}

void DeathCamera::Update()
{
    if (!player_) return;

    // 死亡演出中は世界の時間が止まっているので、実時間で進める
    float dt = DeltaTime::GetDeltaTime();
    zoomTime_ += dt;

    const float t = std::min(zoomTime_ / totalTime_, 1.0f);

    // === シェイク：とどめの瞬間が最大で、寄りきるころには収まる ===
    const float shakeStrength = kMaxShake * (1.0f - t) * (1.0f - t);
    std::uniform_real_distribution<float> shakeDist(-shakeStrength, shakeStrength);
    Vector3 shakeOffset{
        shakeDist(randomEngine_),
        shakeDist(randomEngine_) * 0.7f, // 縦揺れは少し弱めに
        shakeDist(randomEngine_) * 0.3f  // 前後方向は控えめに
    };

    // === 倒れた体を見下ろす位置へ寄る ===
    // 「プレイヤーへ真っ直ぐ寄る」だと、とどめを刺した敵が目の前に立っているぶん
    // カメラが敵の中に入って何も見えなくなる。
    // そこで「水平方向は今の向きのまま少し離れた位置・高さは上」へ回り込み、見下ろす形にする
    Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

    Vector3 flatDir = basePos_ - playerPos;
    flatDir.y = 0.0f;
    flatDir = (Length(flatDir) > 0.001f) ? Normalize(flatDir) : Vector3{ 0.0f, 0.0f, -1.0f };

    const Vector3 target = playerPos + flatDir * kEndDistance + Vector3{ 0.0f, kEndHeight, 0.0f };
    Vector3 cameraPos = Lerp(basePos_, target, easeOutCubic(t)) + shakeOffset;

    GetTranslate() = cameraPos;
    // 足元ぴったりだと画の下端に寄りすぎるので、少しだけ上を見る
    LookAt(playerPos + Vector3{ 0.0f, kLookHeight, 0.0f });

    BaseCamera::Update();
}
