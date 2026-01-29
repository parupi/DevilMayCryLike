#pragma once
#include <GameObject/Character/CharacterStructs.h>

// プレイヤーの物理挙動を計算
class PlayerMovement {
public:
    PlayerMovement() = default;
    ~PlayerMovement() = default;
    // 初期化
    void Initialize();

    // Intent を受け取る
    void SetIntent(const MoveIntent& intent);

    // 毎フレーム更新
    void Update(float deltaTime);

    // 状態を取得
    bool IsGrounded() const { return isGrounded_; }
    void SetIsGrounded(bool flag) { isGrounded_ = flag; }
    const Vector3& GetVelocity() const { return velocity_; }

private:
    // 内部処理
    void UpdateGroundState();
    void ApplyHorizontalMove(float deltaTime);
    void ApplyJump();
    void ApplyGravity(float deltaTime);
    void Integrate(float deltaTime);
    void ClearIntent();

private:
    // ---- 状態 ----
    Vector3 position_;
    Vector3 velocity_;
    bool isGrounded_ = false;

    MoveIntent intent_;

    // ---- パラメータ ----
    float walkSpeed_ = 3.0f;
    float runSpeed_ = 6.0f;
    float jumpPower_ = 8.0f;
    float gravity_ = -9.8f;

    float groundAccel_ = 30.0f;
    float airAccel_ = 10.0f;
};
