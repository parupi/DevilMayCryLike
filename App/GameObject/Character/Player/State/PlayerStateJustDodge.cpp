#include "PlayerStateJustDodge.h"
#include "GameObject/Character/Player/Player.h"

namespace {
	// 空中でジャスト回避したときの落下加速度。PlayerStateDodge と揃えてある
	constexpr float kGravity = -12.0f;
}

void PlayerStateJustDodge::Enter(Player& player) {
	// ダメージ無効・無敵の延長・スローモーション・演出の開始はすべてここから
	player.OnJustDodge();
	// ジャスト回避の直後だけ出せる攻撃（カウンター）の受付を開く
	player.GetCombat()->OpenCounterWindow(player.GetDodgeParams().counterWindow);

	player.GetAcceleration() = { 0.0f, 0.0f, 0.0f };
}

void PlayerStateJustDodge::Update(Player& player, float deltaTime) {
	PlayerDodgeRuntime& runtime = player.GetDodgeRuntime();
	const PlayerDodgeParams& params = player.GetDodgeParams();

	// 演出中も回避は進んでいる扱いにする。止めると「回避 → 停止 → ダッシュ」に見える
	runtime.elapsed += deltaTime;

	Vector3& velocity = player.GetVelocity();
	const float fallSpeed = velocity.y;
	velocity = runtime.direction * params.dodgeSpeed;
	// 空中回避中なら落下も続ける（回避と同じ）。接地していれば縦は止める
	if (player.GetOnGround()) {
		velocity.y = 0.0f;
		player.GetAcceleration().y = 0.0f;
	} else {
		velocity.y = fallSpeed;
		player.GetAcceleration().y = kGravity;
	}

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
