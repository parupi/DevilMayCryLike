#include "PlayerStateAttack1.h"
#include "Vector3.h"

void PlayerStateAttack1::Enter(Player& player)
{
	attackPhase_ = AttackPhase::Raise;
	stateTime_.current = 0.0f;
	stateTime_.max = raiseDuration;

	startRot_ = {};              // 現在の腕の角度を初期値とする
	targetRot_.x = -157.0f;      // 振りかぶり角度

	player.GetPlayerAttackEffect()->InitializeAttackCylinderEffect(raiseDuration + waitDuration + swingDuration);
	player.GetPlayerAttackEffect()->InitializeTargetMarkerEffect(raiseDuration + waitDuration + swingDuration);

   
}

void PlayerStateAttack1::Update(Player& player)
{
    stateTime_.current += 1.0f / 60.0f;
    float t = std::clamp(stateTime_.current / stateTime_.max, 0.0f, 1.0f);

    Quaternion rot;
    switch (attackPhase_)
    {
    case AttackPhase::Raise:
    {
        rot = Slerp(EulerDegree(startRot_), EulerDegree(targetRot_), t);
        if (stateTime_.current >= stateTime_.max) {
            attackPhase_ = AttackPhase::Wait;
            stateTime_.current = 0.0f;
            stateTime_.max = waitDuration;
        }
        break;
    }
    case AttackPhase::Wait:
    {
        rot = Slerp(EulerDegree(startRot_), EulerDegree(targetRot_), 1.0f); // 完全に上がった状態で停止
        if (stateTime_.current >= stateTime_.max) {
            attackPhase_ = AttackPhase::Swing;
            stateTime_.current = 0.0f;
            stateTime_.max = swingDuration;

            // 振り下ろす方向を設定（例えばx = 30度）
            startRot_ = targetRot_;
            targetRot_.x = -30.0f;
            player.GetWeapon()->SetIsAttack(true);
        }
        break;
    }
    case AttackPhase::Swing:
    {

        rot = Slerp(EulerDegree(startRot_), EulerDegree(targetRot_), t);
        if (stateTime_.current >= stateTime_.max) {
            attackPhase_ = AttackPhase::End;
        }
        break;
    }
    case AttackPhase::End:
    {
        player.ChangeState("Idle");
        return;
    }
    }

    player.GetRenderer("PlayerLeftArm")->GetWorldTransform()->GetRotation() = rot;
    player.GetRenderer("PlayerRightArm")->GetWorldTransform()->GetRotation() = rot;
}

void PlayerStateAttack1::Exit(Player& player)
{
	Vector3 newRot{};
	newRot.x = 0.0f;
	player.GetRenderer("PlayerLeftArm")->GetWorldTransform()->GetRotation() = EulerDegree(newRot);
	player.GetRenderer("PlayerRightArm")->GetWorldTransform()->GetRotation() = EulerDegree(newRot);

    player.GetWeapon()->SetIsAttack(false);
}
