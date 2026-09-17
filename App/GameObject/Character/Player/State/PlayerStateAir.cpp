#include "PlayerStateAir.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include "Audio/GameSoundLibrary.h"
#include "Audio/SoundManager.h"

#include <algorithm>

void PlayerStateAir::Enter(Player& player)
{
	// ジャンプから遷移してきた場合はすでに上向きの速度があるので加速度は重力のみ
	player.GetAcceleration().y = -12.0f;
}

void PlayerStateAir::Update(Player& player, float deltaTime)
{
	// 移動方向を取得
	Vector3 moveDir = player.GetMoveDirection();
	// 移動と回転を実行
	player.Move(moveDir, deltaTime);
	player.Rotate(moveDir, deltaTime);

	// 地面についたら待機状態にする
	if (player.GetOnGround()) {
		// 落ちた勢いが強いほど着地音を大きくする。
		// Exit で velocity を捨てるので、遷移より前にここで見ておくこと
		const float fallSpeed = -player.GetVelocity().y;
		const float volume = std::clamp(0.35f + fallSpeed * 0.045f, 0.35f, 0.9f);
		SoundManager::GetInstance().PlaySE(GameSound::kPlayerLand, volume);

 		player.ChangeState("Idle");
		return;
	}
}

void PlayerStateAir::Exit(Player& player)
{
	player.GetVelocity() = { 0.0f, 0.0f, 0.0f };
}

void PlayerStateAir::ExecuteCommand(Player&, const PlayerCommand&)
{

}
