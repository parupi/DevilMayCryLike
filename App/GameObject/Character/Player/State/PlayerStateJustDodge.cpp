#include "PlayerStateJustDodge.h"
#include "GameObject/Character/Player/Player.h"

void PlayerStateJustDodge::Enter(Player& player) {
	// ダメージ無効・無敵の延長・スローモーション・演出の開始はすべてここから
	player.OnJustDodge();

	player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };
}

void PlayerStateJustDodge::Update(Player& player, float deltaTime) {
	PlayerDodgeRuntime& runtime = player.GetDodgeRuntime();
	const PlayerDodgeParams& params = player.GetDodgeParams();

	// 演出中も回避は進んでいる扱いにする。止めると「回避 → 停止 → ダッシュ」に見える
	runtime.elapsed += deltaTime;

	Vector3& velocity = player.GetVelocity();
	velocity = runtime.direction * params.dodgeSpeed;
	velocity.y = 0.0f;

	// 終了判定はスローモーションの残り時間で見る。
	// スロー中は deltaTime 自体が縮んでいるので、ここで自前のタイマーを積むと演出が伸びる
	if (player.IsJustDodgeSlow()) return;

	// 回避が残っていれば回避へ戻り、使い切っていればダッシュへ発展する
	if (runtime.elapsed < params.dodgeDuration) {
		runtime.resuming = true;
		player.ChangeState("Dodge");
	} else {
		player.ChangeState("Dash");
	}
}

void PlayerStateJustDodge::Exit(Player&) {
	// 速度は遷移先（Dodge / Dash）がそのまま引き継ぐ
}

void PlayerStateJustDodge::ExecuteCommand(Player&, const PlayerCommand&) {
	// 0.08秒の演出中は入力を受け付けない
}
