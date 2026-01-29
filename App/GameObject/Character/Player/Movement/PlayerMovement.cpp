#include "PlayerMovement.h"
#include <algorithm>

void PlayerMovement::Initialize()
{
}

void PlayerMovement::SetIntent(const MoveIntent& intent)
{
    intent_ = intent;
}

void PlayerMovement::Update(float deltaTime)
{
    UpdateGroundState();

    ApplyHorizontalMove(deltaTime);

    ApplyJump();

    ApplyGravity(deltaTime);

    Integrate(deltaTime);

    ClearIntent();
}

void PlayerMovement::UpdateGroundState()
{

}

void PlayerMovement::ApplyHorizontalMove(float deltaTime)
{
    Vector3 dir = intent_.moveDir;
    if (Length(dir) <= 0.0f) return;

    float targetSpeed = runSpeed_ * intent_.moveScale;

    Vector3 targetVel = dir * targetSpeed;
    Vector3 currentVel = velocity_;
    currentVel.y = 0.0f;

    float accel = isGrounded_ ? groundAccel_ : airAccel_;

    Vector3 delta = targetVel - currentVel;
    Vector3 accelStep = ClampLength(delta, accel * deltaTime);

    velocity_ += accelStep;
}

void PlayerMovement::ApplyJump()
{
    if (!intent_.jump) return;
    if (!isGrounded_) return;

    velocity_.y = jumpPower_;
    isGrounded_ = false;
}

void PlayerMovement::ApplyGravity(float deltaTime)
{
    if (!isGrounded_) {
        velocity_.y += gravity_ * deltaTime;
    }
}

void PlayerMovement::Integrate(float deltaTime)
{
    position_ += velocity_ * deltaTime;
}

void PlayerMovement::ClearIntent()
{
    intent_ = {};
}
