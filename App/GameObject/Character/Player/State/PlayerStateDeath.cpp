#include "PlayerStateDeath.h"

#include <algorithm>

#include <Audio/SoundManager.h>
#include <GameObject/Camera/DeathCamera.h>
#include <Math/Easing.h>
#include <Scene/Transition/TransitionManager.h>
#include <Scene/Transition/VignetteExpandTransition.h>
#include <World3D/Camera/CameraManager.h>
#include <World3D/Object/Model/Animation/AnimationPlayer.h>

#include "GameObject/Character/Player/Player.h"

PlayerStateDeath::PlayerStateDeath()
{
	// ステート生成時に一度トランジションを生成しておく（リトライのシーン切り替えで使う）
	if (TransitionManager::GetInstance().AddTransition(std::make_unique<VignetteExpandTransition>("Death"))) {
		// 追加に成功したら初期化
		static_cast<VignetteExpandTransition*>(TransitionManager::GetInstance().GetCurrentTransition())->Initialize();
		// 生成した段階ではフェードをセットしておく
		TransitionManager::GetInstance().SetTransition("Fade");
	}
}

void PlayerStateDeath::Enter(Player& player)
{
	BeginPhase(Phase::Fall);
	totalTimer_ = 0.0f;
	motionDuration_ = 0.0f;

	// 死亡演出の間にBGMを引かせる（この後 GAMEPLAY を読み直して鳴り始める）
	SoundManager::GetInstance().StopBGM(kBgmFadeTime);
	// 死亡SE。resource/sound に PlayerDeath.wav を置けば鳴る（無い場合は黙って何も起きない）
	SoundManager::GetInstance().PlaySE("PlayerDeath", 1.0f);

	// ── カメラを寄せる ──
	CameraManager& cameraManager = CameraManager::GetInstance();
	if (BaseCamera* current = cameraManager.GetActiveCamera()) {
		cameraManager.AddCamera(std::make_unique<DeathCamera>("DeathCamera", current, &player));
		cameraManager.SetActiveCamera("DeathCamera");
	}

	// ── とどめの吹き飛び ──
	// KnockBack ステートを経由しないので、とどめの一撃の情報から初速をここで直接与える。
	// 空中コンボのように吹き飛ばしが 0 の攻撃で終わることもあるので下限を設けている
	const DamageInfo& info = player.GetPendingDamageInfo();
	Vector3 horizontal{ info.direction.x, 0.0f, info.direction.z };
	if (Length(horizontal) > 0.001f) {
		horizontal = Normalize(horizontal);
	} else {
		horizontal = {};
	}
	const float speed = (std::max)(info.impulseForce, kLaunchMinSpeed);
	const float upSpeed = (std::max)(info.impulseForce * info.upwardRatio, kLaunchMinUpSpeed);

	Vector3& velocity = player.GetVelocity();
	velocity = horizontal * speed;
	velocity.y = upSpeed;
	// 重力はこのステートが自分で足すので、加速度は切っておく（二重に落ちるのを防ぐ）
	player.GetAcceleration() = {};

	// HUDはここでは消さない。UIレイヤーごと隠すとゲームオーバーの選択肢まで消えるので、
	// 演出に合わせて Update でフェードアウトさせる
	player.SetHudAlpha(1.0f);
}

void PlayerStateDeath::Update(Player& player, float deltaTime)
{
	totalTimer_ += deltaTime;
	phaseTimer_ += deltaTime;

	// ── 吹き飛び → 着地 ──
	// 位置への足し込みは Player::Update がやるので、ここでは重力と減衰だけ見る
	Vector3& velocity = player.GetVelocity();
	if (player.GetOnGround()) {
		// 接地したら落下速度を殺す。足し続けると地面へめり込もうとして倒れた体が震える
		if (velocity.y < 0.0f) {
			velocity.y = 0.0f;
		}
	} else {
		velocity.y += kGravity * deltaTime;
	}
	const float damp = (std::max)(0.0f, 1.0f - kHorizontalDamp * deltaTime);
	velocity.x *= damp;
	velocity.z *= damp;

	// ── HUDをフェードアウト ──
	player.SetHudAlpha(1.0f - std::clamp(totalTimer_ / kHudFadeDuration, 0.0f, 1.0f));

	// 死亡クリップの長さは Player::UpdateAnimation が流し始めてからでないと取れないので、
	// 取れるまで毎フレーム試す
	if (motionDuration_ <= 0.0f) {
		motionDuration_ = ResolveMotionDuration(player);
	}
	const float fallDuration = (motionDuration_ > 0.0f) ? motionDuration_ : kFallbackMotionDuration;
	const float totalDuration = fallDuration + kLingerDuration + kDissolveDuration;

	// ── 画面から色を抜いて視界を閉じる ──
	// 倒れるところは見せたいので、効きは後半へ寄せる
	if (DeathScreenEffect* screen = player.GetDeathScreen()) {
		const float t = std::clamp(totalTimer_ / totalDuration, 0.0f, 1.0f);
		screen->SetProgress(easeInQuad(t));
	}

	switch (phase_) {
	case Phase::Fall:
		// 死亡モーションを最後まで流し切る（クリップが終わったら最後のポーズで止まる）
		if (phaseTimer_ >= fallDuration) {
			BeginPhase(Phase::Linger);
		}
		break;

	case Phase::Linger:
		// 倒れたポーズのまま少し見せてから消し始める
		if (phaseTimer_ >= kLingerDuration) {
			BeginPhase(Phase::Dissolve);
			if (DissolveOutEffect* dissolve = player.GetDeathDissolve()) {
				dissolve->SetDuration(kDissolveDuration);
				dissolve->Start();
			}
		}
		break;

	case Phase::Dissolve: {
		DissolveOutEffect* dissolve = player.GetDeathDissolve();
		if (dissolve) {
			dissolve->Update(deltaTime, player.GetWorldTransform()->GetTranslation());
		}
		// 溶けきったら終了。エフェクトが無い構成でも止まらないよう時間でも抜ける
		if (!dissolve || dissolve->IsFinished() || phaseTimer_ >= kDissolveDuration) {
			BeginPhase(Phase::Finished);
			// ここでシーンを切り替えてしまうと、やり直すかタイトルへ戻るかを選ぶ余地が無くなる。
			// 印だけ付けて、あとは GameScene（GameOver ステート）に任せる
			player.NotifyDeathFinished();
		}
		break;
	}

	case Phase::Finished:
		// 消えきった状態を保つ。ステートは Death のままにしておくことで、
		// ゲームオーバーの選択中も入力を受けず、世界も止まったままになる
		break;
	}
}

void PlayerStateDeath::Exit(Player& player)
{
	// 通常は死んだらこのステートから抜けないが、外から戻された場合に
	// 画面が灰色のまま・体が溶けたまま残らないように片付ける
	player.SetHudAlpha(1.0f);
	if (DeathScreenEffect* screen = player.GetDeathScreen()) {
		screen->Reset();
	}
	if (DissolveOutEffect* dissolve = player.GetDeathDissolve()) {
		dissolve->Reset();
	}
}

void PlayerStateDeath::ExecuteCommand(Player&, const PlayerCommand&)
{
}

void PlayerStateDeath::BeginPhase(Phase phase)
{
	phase_ = phase;
	phaseTimer_ = 0.0f;
}

float PlayerStateDeath::ResolveMotionDuration(Player& player) const
{
	AnimationPlayer* anim = player.GetAnimationPlayer();
	if (!anim) return 0.0f;
	// 死亡クリップに切り替わるまでは、直前のクリップの長さが返ってくるので弾く
	if (anim->GetCurrentClipName() != Player::kClipDeath) return 0.0f;
	return anim->GetDuration();
}
