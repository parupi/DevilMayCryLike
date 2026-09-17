#include "PlayerStateDodge.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"

namespace {
	// 空中に出てしまったときの落下加速度。他のステートと揃えてある
	constexpr float kGravity = -12.0f;
}

void PlayerStateDodge::Enter(Player& player) {
	PlayerDodgeRuntime& runtime = player.GetDodgeRuntime();

	// ジャスト回避から戻ってきた場合は、方向も経過時間もそのまま引き継ぐ。
	// ここでやり直すと「回避 → 停止 → 回避」に見えてしまう
	if (!runtime.resuming) {
		runtime.direction = player.CalcDodgeDirection();
		runtime.elapsed = 0.0f;
		runtime.justDodgeUsed = false;
		runtime.justDodgePending = false;

		player.FaceDirection(runtime.direction);
		player.OnDodgeStart();
	}
	runtime.resuming = false;

	player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };
}

void PlayerStateDodge::Update(Player& player, float deltaTime) {
	PlayerDodgeRuntime& runtime = player.GetDodgeRuntime();
	const PlayerDodgeParams& params = player.GetDodgeParams();

	runtime.elapsed += deltaTime;

	// 回避中は通常の移動入力で方向を変えない（仕様書 §5.3）。
	// 「入力した方向へ素早く移動する」明確なアクションとして扱うため
	Vector3& velocity = player.GetVelocity();
	const float fallSpeed = velocity.y;
	velocity = runtime.direction * params.dodgeSpeed;

	// 崖から出た場合だけ落下させる。接地している間は縦速度を殺しておく
	if (player.GetOnGround()) {
		velocity.y = 0.0f;
		player.GetAcceleration().y = 0.0f;
	} else {
		velocity.y = fallSpeed;
		player.GetAcceleration().y = kGravity;
	}

	// 被弾側(Player::TakeDamage)が立てた印を拾う。ダメージより先に判定する（仕様書 §15）
	if (runtime.justDodgePending) {
		player.ChangeState("JustDodge");
		return;
	}

	// 回避時間を使い切ったらダッシュへ
	if (runtime.elapsed >= params.dodgeDuration) {
		player.ChangeState("Dash");
		return;
	}
}

void PlayerStateDodge::Exit(Player&) {
	// 速度はダッシュ／ジャスト回避がそのまま引き継ぐので、ここでは触らない。
	// 通常状態へ戻る経路（ノックバック・死亡）は、遷移先が自分で速度を設定する
}

void PlayerStateDodge::ExecuteCommand(Player&, const PlayerCommand&) {
	// 回避中は他の行動を受け付けない。0.25秒で終わるので入力の取りこぼしにはならない。
	// 回避の入力だけは Player::ExecuteCommand が直接処理する（攻撃中でもキャンセルできるようにするため）
}
