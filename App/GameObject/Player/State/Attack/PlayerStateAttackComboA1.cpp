#include "PlayerStateAttackComboA1.h"
#include "math/Vector3.h"
#include <base/utility/DeltaTime.h>
#include <3d/Collider/AABBCollider.h>
#include "math/Quaternion.h"

PlayerStateAttackComboA1::PlayerStateAttackComboA1(std::string attackName) : PlayerStateAttackBase(attackName)
{

}

void PlayerStateAttackComboA1::Enter(Player& player)
{
	PlayerStateAttackBase::Enter(player);
	cancelTime_ = 0.0f;
}

void PlayerStateAttackComboA1::Update(Player& player)
{
	PlayerStateAttackBase::Update(player);

	cancelTime_ += DeltaTime::GetDeltaTime();

	if (attackPhase_ == AttackPhase::Cancel) {
		if (cancelTime_ < 0.45f) {
			if (Input::GetInstance()->TriggerKey(DIK_J)) {
				player.ChangeState("AttackComboA2");
			}
		} else {
			if (Input::GetInstance()->TriggerKey(DIK_J)) {
				player.ChangeState("AttackComboB2");
			}
		}

		// 攻撃のトリガー
		if (Input::GetInstance()->TriggerKey(DIK_H)) {
			player.ChangeState("AttackHighTime");
			return;
		}

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			player.ChangeState("Jump");
		}
	}
}

void PlayerStateAttackComboA1::Exit(Player& player)
{
	PlayerStateAttackBase::Exit(player);
}
