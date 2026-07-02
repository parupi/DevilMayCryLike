#include "PlayerStateAttack.h"
#include "Debugger/GlobalVariables.h"
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#include <Math/MathUtils.h>
#include "GameObject/Character/Player/Player.h"
#include "World3D/Collider/AABBCollider.h"
#include "Utility/DeltaTime.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"

PlayerStateAttack::PlayerStateAttack(std::string attackName) {
	name_ = attackName;
	std::string groupName = "Attack/" + name_;
	// 攻撃のデータを生成
	gv->CreateGroup(groupName);

	gv->AddItem(name_, "PointCount", int32_t()); // 制御点の数
	// 移動系
	gv->AddItem(name_, "MoveSpeed", Vector3());          // 攻撃中にどれくらい移動するか
	gv->AddItem(name_, "KnockBackSpeed", Vector3());     // 敵のノックバック

	// タイマー系
	gv->AddItem(name_, "TotalDuration", float());      // 攻撃全体にかかる時間
	gv->AddItem(name_, "PreDelay", float());           // 予備動作の時間
	gv->AddItem(name_, "AttackDuration", float());     // 攻撃にかかる時間
	gv->AddItem(name_, "PostDelay", float());          // 技後の硬直時間
	gv->AddItem(name_, "NextAttackDelay", float());    // 次の攻撃が出せるまでの時間

	// その他
	gv->AddItem(name_, "DrawDebugControlPoints", bool()); // 制御点のデバッグ描画フラグ
	gv->AddItem(name_, "IsMove", bool(false)); // 攻撃中に移動するかどうか(モーション用のフラグ)

	// ダメージなどの汎用パラメータ
	gv->AddItem(name_, "Damage", float());

	// 派生先インデックスの個数
	gv->AddItem(name_, "NextAttackCount", int32_t(0));

	// 初期で最大3個まで確保（必要なら動的にも増やせる）
	for (int i = 0; i < 3; ++i) {
		gv->AddItem(name_, "NextAttack_" + std::to_string(i), std::string("")); // -1 = 無効
	}

	gv->AddItem(name_, "AttackPosture", int32_t(0));

	gv->AddItem(name_, "HitStopTime", float());

	gv->AddItem(name_, "HitStopIntensity", float());

	// 攻撃を受けた側に送る情報
	gv->AddItem(name_, "ReactionType", int32_t(0));
	// ノックバック＆打ち上げ共通
	gv->AddItem(name_, "ImpulseForce", float());
	gv->AddItem(name_, "UpwardRatio", float());
	// 吹っ飛び用
	gv->AddItem(name_, "TorqueForce", float());
	// のけぞり用
	gv->AddItem(name_, "StunTime", float());

	gv->AddItem(name_, "ButtonIndex", int32_t(0));
	gv->AddItem(name_, "LockOnFlag", bool(false));
	gv->AddItem(name_, "RootAttackFlag", bool(false));
	gv->AddItem(name_, "IsAir", bool(false));
	gv->AddItem(name_, "DirIndex", int32_t(0));
}

void PlayerStateAttack::Enter(Player& player) {
	attackPhase_ = AttackPhase::Startup;
	stateTime_.current = 0.0f;
	stateTime_.max = attackData_.totalDuration;

	attackData_.pointCount = gv->GetValueRef<int32_t>(name_, "PointCount");

	attackChangeTimer_.max = attackData_.nextAttackDelay;
	attackChangeTimer_.current = 0.0f;

	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints.push_back(gv->GetValueRef<Vector3>(name_, "ControlPoint_" + std::to_string(i)));
		attackData_.controlRotations.push_back(gv->GetValueRef<Vector3>(name_, "ControlRotation_" + std::to_string(i)));
	}
	// 今回の攻撃のパラメータを送っておく
	player.SetAttackData(attackData_);

	isFinish_ = false;
}

void PlayerStateAttack::Update(Player& player, float deltaTime) {
	stateTime_.current += deltaTime;

	// 攻撃フェーズの更新処理
	UpdatePhase(stateTime_.current);

	// フェーズごとの処理を追加
	switch (attackPhase_) {
	case AttackPhase::Startup:
	{
		UpdateStartup(player, deltaTime);
		break;
	}
	case AttackPhase::Active:
	{
		UpdateActive(player);
		break;
	}
	case AttackPhase::Recovery:
		UpdateRecovery(player);
		break;
	case AttackPhase::Cancel:
		attackChangeTimer_.current += deltaTime;

		// 状態遷移
		if (stateTime_.current >= attackData_.totalDuration) {
			isFinish_ = true;
		}

		break;
	}

	// 移動可能の場合は移動させる
	if (attackData_.isMove) {
		player.Move(player.GetMoveDirection(), deltaTime);
	}
}

void PlayerStateAttack::Exit(Player& player) {
	player;
	attackPhase_ = AttackPhase::Startup;
	stateTime_.current = 0.0f;
	stateTime_.max = attackData_.totalDuration;

	attackChangeTimer_.current = 0.0f;
}

AttackRequestData PlayerStateAttack::ExecuteCommand(Player& player, const PlayerCommand& command) {
	AttackRequestData req{};
	req.nextAttack = "";
	req.type = AttackRequest::None;

	if (command.action == PlayerAction::Attack) {
		if (attackPhase_ != AttackPhase::Cancel) {
			return req;
		}

		const AttackNode& node = player.GetCombat()->GetAttackNode(name_);

		int nextCount = static_cast<int>(node.nextAttacks.size());

		if (nextCount > 0 && attackChangeTimer_.max > 0.0f) {

			float segment = attackChangeTimer_.max / static_cast<float>(nextCount);
			float elapsed = attackChangeTimer_.current;

			// どの派生か（0 ～ nextCount-1）
			int derivedIndex = static_cast<int>(elapsed / segment);

			// 範囲外防止
			derivedIndex = std::clamp(derivedIndex, 0, nextCount - 1);

			// 次の派生の名前を取得
			const std::string& nextAttackName = node.nextAttacks[derivedIndex];

			req.nextAttack = nextAttackName;
			req.type = AttackRequest::ChangeAttack;
			return req;
		}

	}
	else if (command.action == PlayerAction::Jump) {
		if (attackPhase_ != AttackPhase::Cancel || gv->GetValueRef<int32_t>(name_, "AttackPosture") == 1) {
			return req;
		}

		req.type = AttackRequest::Jump;
		return req;
	}

	return req;
}

void PlayerStateAttack::OnInterrupted(Player&) {
	// 割り込みされたので攻撃を終了させる
	isFinish_ = true;
}

void PlayerStateAttack::UpdateAttackData() {
	// 制御点
	attackData_.pointCount = GlobalVariables::GetInstance().GetValueRef<int32_t>(name_, "PointCount");
	attackData_.controlPoints.resize(attackData_.pointCount);
	attackData_.controlRotations.resize(attackData_.pointCount);
	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints[i] = GlobalVariables::GetInstance().GetValueRef<Vector3>(name_, "ControlPoint_" + std::to_string(i));
		attackData_.controlRotations[i] = GlobalVariables::GetInstance().GetValueRef<Vector3>(name_, "ControlRotation_" + std::to_string(i));
	}

	// 移動系
	attackData_.moveVelocity = GlobalVariables::GetInstance().GetValueRef<Vector3>(name_, "MoveSpeed");
	attackData_.knockBackSpeed = GlobalVariables::GetInstance().GetValueRef<Vector3>(name_, "KnockBackSpeed");

	// タイマー系
	attackData_.totalDuration = GlobalVariables::GetInstance().GetValueRef<float>(name_, "TotalDuration");
	attackData_.preDelay = GlobalVariables::GetInstance().GetValueRef<float>(name_, "PreDelay");
	attackData_.attackDuration = GlobalVariables::GetInstance().GetValueRef<float>(name_, "AttackDuration");
	attackData_.postDelay = GlobalVariables::GetInstance().GetValueRef<float>(name_, "PostDelay");
	attackData_.nextAttackDelay = GlobalVariables::GetInstance().GetValueRef<float>(name_, "NextAttackDelay");

	// その他
	attackData_.isMove = GlobalVariables::GetInstance().GetValueRef<bool>(name_, "IsMove");
	attackData_.drawDebugControlPoints = GlobalVariables::GetInstance().GetValueRef<bool>(name_, "DrawDebugControlPoints");
	attackData_.damage = GlobalVariables::GetInstance().GetValueRef<float>(name_, "Damage");

	attackData_.hitStopTime = GlobalVariables::GetInstance().GetValueRef<float>(name_, "HitStopTime");

	attackData_.hitStopIntensity = GlobalVariables::GetInstance().GetValueRef<float>(name_, "HitStopIntensity");
	// 攻撃を受けた側に送る情報
	attackData_.type = static_cast<ReactionType>(GlobalVariables::GetInstance().GetValueRef<int32_t>(name_, "ReactionType"));
	// ノックバック＆打ち上げ共通
	attackData_.impulseForce = GlobalVariables::GetInstance().GetValueRef<float>(name_, "ImpulseForce");
	attackData_.upwardRatio = gv->GetValueRef<float>(name_, "UpwardRatio");
	// 吹っ飛び用
	attackData_.torqueForce = gv->GetValueRef<float>(name_, "TorqueForce");
	// のけぞり用
	attackData_.stunTime = gv->GetValueRef<float>(name_, "StunTime");
}

void PlayerStateAttack::DrawControlPoints(Player& player) {
	if (attackData_.pointCount < 4 || !attackData_.drawDebugControlPoints) return;

	// 制御点の位置に球を描画
	for (int32_t i = 0; i < attackData_.pointCount; ++i) {
		PrimitiveLineDrawer::GetInstance().DrawWireSphere(player.GetWorldTransform()->GetTranslation() + attackData_.controlPoints[i], 0.05f, { 1, 1, 1, 1 }, 24);
	}

	// 曲線がどんな感じになるか計算
	std::vector<Vector3> curvePoints;
	if (attackData_.pointCount >= 4) {
		const float step = 0.1f;
		for (float t = 0.0f; t <= 1.0f; t += step) {
			Vector3 point = CatmullRomSpline(attackData_.controlPoints, t);
			curvePoints.push_back(point);
		}
	}
	// 計算した曲線を描画
	for (size_t i = 0; i < curvePoints.size() - 1; i++) {
		PrimitiveLineDrawer::GetInstance().DrawLine(player.GetWorldTransform()->GetTranslation() + curvePoints[i], player.GetWorldTransform()->GetTranslation() + curvePoints[i + 1], { 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

bool PlayerStateAttack::HasBranch(Player& player) const {
	const AttackNode& node = player.GetCombat()->GetAttackNode(name_);

	int nextCount = static_cast<int>(node.nextAttacks.size());

	return nextCount > 0;
}

void PlayerStateAttack::UpdatePhase(float time) {
	if (time < attackData_.preDelay) {
		attackPhase_ = AttackPhase::Startup;
	}
	else if (time < attackData_.preDelay + attackData_.attackDuration) {
		attackPhase_ = AttackPhase::Active;
	}
	else if (time < attackData_.preDelay + attackData_.attackDuration + attackData_.postDelay) {
		attackPhase_ = AttackPhase::Recovery;
	}
	else {
		attackPhase_ = AttackPhase::Cancel;
	}
}

void PlayerStateAttack::UpdateStartup(Player& player, float deltaTime) {
	// 方向転換
	Vector3 moveDir = player.GetMoveDirection();
	// プレイヤーの向きを移動方向に向ける
	player.Rotate(moveDir, deltaTime);

	// 武器を制御点の最初の位置に移動させる
	Vector3 targetPos = attackData_.controlPoints[0];
	// 武器の初期位置
	Vector3 firstPos = player.GetWeapon()->GetWorldTransform()->GetTranslation();
	// 予備動作の時間に応じて線形補間で移動させる
	Vector3 currentPos = Lerp(firstPos, targetPos, stateTime_.current / attackData_.preDelay);
	// 武器の位置を更新
	player.GetWeapon()->GetWorldTransform()->GetTranslation() = currentPos;
}

void PlayerStateAttack::UpdateActive(Player& player) {
	// 攻撃判定ON、移動処理
	player.GetWeapon()->SetIsAttack(true);
	// ローカル速度の取得
	Vector3 localVelocity = attackData_.moveVelocity;
	// プレイヤーの回転を取得
	Quaternion rotation = player.GetWorldTransform()->GetRotation();
	// ローカル速度をワールド座標に変換
	Vector3 worldVelocity = RotateVector(localVelocity, rotation);
	// 速度を設定
	player.GetVelocity() = worldVelocity;

	// 正規化t
	float activeStart = attackData_.preDelay;
	float t = std::clamp((stateTime_.current - activeStart) / attackData_.attackDuration, 0.0f, 1.0f);

	// 現在位置を計算
	Vector3 pos = CatmullRomSpline(attackData_.controlPoints, t);
	// 現在の回転を計算
	Vector3 rot = CatmullRomSpline(attackData_.controlRotations, t);

	// 位置の設定
	player.GetWeapon()->GetWorldTransform()->GetTranslation() = pos;
	// 回転の設定
	player.GetWeapon()->GetWorldTransform()->GetRotation() = EulerDegree(rot);
}

void PlayerStateAttack::UpdateRecovery(Player& player) {
	player.GetWeapon()->SetIsAttack(false);

	player.GetVelocity() = { 0.0f, 0.0f, 0.0f };
}

AttackRequestData PlayerStateAttack::UpdateCancel(Player& player) {
	player;
	return AttackRequestData();
}
