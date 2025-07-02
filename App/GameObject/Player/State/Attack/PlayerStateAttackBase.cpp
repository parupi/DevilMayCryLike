#include "PlayerStateAttackBase.h"
#include <debuger/GlobalVariables.h>
#include <3d/Primitive/PrimitiveLineDrawer.h>
#include <math/function.h>
#include "GameObject/Player/Player.h"
#include <3d/Collider/AABBCollider.h>
#include <base/utility/DeltaTime.h>

PlayerStateAttackBase::PlayerStateAttackBase(std::string attackName)
{
	name_ = attackName;

	// 攻撃のデータを生成
	gv->CreateGroup(name_);

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

	// ダメージなどの汎用パラメータ
	gv->AddItem(name_, "Damage", float());
}

void PlayerStateAttackBase::Enter(Player& player)
{
	attackPhase_ = AttackPhase::Startup;
	stateTime_.current = 0.0f;
	stateTime_.max = attackData_.totalDuration;

	attackData_.pointCount = gv->GetValueRef<int32_t>(name_, "PointCount");

	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints.push_back(gv->GetValueRef<Vector3>(name_, "ControlPoint_" + std::to_string(i)));
	}
	// 今回の攻撃のパラメータを送っておく
	player.SetAttackData(attackData_);
}

void PlayerStateAttackBase::Update(Player& player)
{
	stateTime_.current += DeltaTime::GetDeltaTime();

	// 攻撃フェーズの更新処理
	float time = stateTime_.current;

	// 攻撃の時間によってフェーズを管理
	if (time < attackData_.preDelay) {
		attackPhase_ = AttackPhase::Startup;
	} else if (time < attackData_.preDelay + attackData_.attackDuration) {
		attackPhase_ = AttackPhase::Active;
	} else if (time < attackData_.preDelay + attackData_.attackDuration + attackData_.postDelay) {
		attackPhase_ = AttackPhase::Recovery;
	} else if (time < attackData_.totalDuration) {
		attackPhase_ = AttackPhase::Cancel;
	}
	// フェーズごとの処理を追加
	switch (attackPhase_) {
	case AttackPhase::Startup:

		break;
	case AttackPhase::Active:
	{
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
		float t = std::clamp((time - activeStart) / attackData_.attackDuration, 0.0f, 1.0f);

		// 現在位置と進行方向
		Vector3 pos = CatmullRomSpline(attackData_.controlPoints, t);
		const float deltaT = 0.01f;
		float tNext = std::clamp(t + deltaT, 0.0f, 1.0f);
		Vector3 nextPos = CatmullRomSpline(attackData_.controlPoints, tNext);

		Vector3 forward = Normalize(nextPos - pos); // 曲線の接線
		Vector3 up = { 0.0f, 1.0f, 0.0f };

		// 曲線に直角な方向（right）を主軸にした回転にする
		Vector3 right = Normalize(Cross(up, forward));       // ← forwardと垂直
		up = Normalize(Cross(forward, right));               // 正規直交系再構成

		// 回転行列を right/up/forward の順で構築
		Matrix4x4 rotationMat = {
			right.x,  right.y,  right.z,  0.0f,
			up.x,     up.y,     up.z,     0.0f,
			forward.x,forward.y,forward.z,0.0f,
			0.0f,     0.0f,     0.0f,     1.0f
		};

		// 回転クォータニオン化
		Quaternion qRot = QuaternionFromMatrix(rotationMat);
		player.GetWeapon()->GetWorldTransform()->GetRotation() = qRot;

		// 位置の設定
		player.GetWeapon()->GetWorldTransform()->GetTranslation() = pos;
		break;
	}
	case AttackPhase::Recovery:
		player.GetWeapon()->SetIsAttack(false);

		player.GetVelocity() = { 0.0f, 0.0f, 0.0f };
		break;
	}

	// 状態遷移
	if (time >= attackData_.totalDuration) {
		player.ChangeState("Air");
	}
}

void PlayerStateAttackBase::Exit(Player& player)
{
	player;
}

void PlayerStateAttackBase::UpdateAttackData()
{
	// 制御点
	attackData_.pointCount = gv->GetInstance()->GetValueRef<int32_t>(name_, "PointCount");
	attackData_.controlPoints.resize(attackData_.pointCount);
	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints[i] = gv->GetInstance()->GetValueRef<Vector3>(name_, "ControlPoint_" + std::to_string(i));
	}

	// 移動系
	attackData_.moveVelocity = gv->GetInstance()->GetValueRef<Vector3>(name_, "MoveSpeed");
	attackData_.knockBackSpeed = gv->GetInstance()->GetValueRef<Vector3>(name_, "KnockBackSpeed");

	// タイマー系
	attackData_.totalDuration = gv->GetInstance()->GetValueRef<float>(name_, "TotalDuration");
	attackData_.preDelay = gv->GetInstance()->GetValueRef<float>(name_, "PreDelay");
	attackData_.attackDuration = gv->GetInstance()->GetValueRef<float>(name_, "AttackDuration");
	attackData_.postDelay = gv->GetInstance()->GetValueRef<float>(name_, "PostDelay");
	attackData_.nextAttackDelay = gv->GetInstance()->GetValueRef<float>(name_, "NextAttackDelay");

	// その他
	attackData_.drawDebugControlPoints = gv->GetInstance()->GetValueRef<bool>(name_, "DrawDebugControlPoints");
	attackData_.damage = gv->GetInstance()->GetValueRef<float>(name_, "Damage");
}

void PlayerStateAttackBase::DrawControlPoints(Player& player)
{
	if (attackData_.pointCount < 4 || !attackData_.drawDebugControlPoints) return;

	// 制御点の位置に球を描画
	for (int32_t i = 0; i < attackData_.pointCount; ++i) {
		PrimitiveLineDrawer::GetInstance()->DrawWireSphere(player.GetWorldTransform()->GetTranslation() + attackData_.controlPoints[i], 0.05f, { 1, 1, 1, 1 }, 24);
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
		PrimitiveLineDrawer::GetInstance()->DrawLine(player.GetWorldTransform()->GetTranslation() + curvePoints[i], player.GetWorldTransform()->GetTranslation() + curvePoints[i + 1], { 1.0f, 1.0f, 1.0f, 1.0f });
	}
}