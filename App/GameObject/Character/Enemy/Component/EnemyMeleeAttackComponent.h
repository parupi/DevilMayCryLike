#pragma once
#include <math/Vector3.h>
#include <vector>

class Enemy;
class Object3d;

struct MeleeAttackParams {
    float windupDuration  = 0.5f;   // 予備動作: 構えポーズを保持する時間
    float attackDuration  = 0.25f;  // 攻撃モーション: CatmullRom スイングの時間
    float rushSpeed       = 0.0f;   // > 0 なら攻撃フェーズ中プレイヤーへ突進

    // weaponTranslate[1] と weaponRotate[1] が構えポーズ（t=0）になる
    std::vector<Vector3> weaponTranslate;
    std::vector<Vector3> weaponRotate;
};

/// <summary>
/// 近接攻撃実行コンポーネント。
/// 予備動作（Windup）フェーズで構えを保持した後、攻撃フェーズでスイングする。
/// </summary>
class EnemyMeleeAttackComponent {
public:
    explicit EnemyMeleeAttackComponent(Object3d* weapon);

    void BeginAttack(Enemy& enemy, const MeleeAttackParams& params);
    void Update(Enemy& enemy, float deltaTime);

    bool IsFinished()  const { return finished_; }
    bool IsWindingUp() const { return !finished_ && timer_ < params_.windupDuration; }

private:
    void ApplyWeaponPose(float t);

    Object3d*         weapon_;
    MeleeAttackParams params_;
    float             timer_    = 0.0f;
    bool              finished_ = true;
};
