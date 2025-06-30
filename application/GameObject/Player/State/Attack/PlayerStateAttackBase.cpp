#include "PlayerStateAttackBase.h"
#include <GlobalVariables.h>
#include <Primitive/PrimitiveLineDrawer.h>
#include <function.h>
#include "GameObject/Player/Player.h"

PlayerStateAttackBase::PlayerStateAttackBase(std::string attackName)
{
	name = attackName;

	// 攻撃のデータを生成
	gv->CreateGroup(name);

	gv->AddItem(name, "PointCount", int32_t()); // 制御点の数
	// 移動系
	gv->AddItem(name, "MoveSpeed", float());          // 攻撃中にどれくらい移動するか
	gv->AddItem(name, "KnockBackSpeed", float());     // 敵のノックバック

	// タイマー系
	gv->AddItem(name, "TotalDuration", float());      // 攻撃全体にかかる時間
	gv->AddItem(name, "PreDelay", float());           // 予備動作の時間
	gv->AddItem(name, "AttackDuration", float());     // 攻撃にかかる時間
	gv->AddItem(name, "PostDelay", float());          // 技後の硬直時間
	gv->AddItem(name, "NextAttackDelay", float());    // 次の攻撃が出せるまでの時間

	// その他
	gv->AddItem(name, "DrawDebugControlPoints", bool()); // 制御点のデバッグ描画フラグ

	// ダメージなどの汎用パラメータ
	gv->AddItem(name, "Damage", float());
}

void PlayerStateAttackBase::UpdateAttackData()
{
	// 制御点
	attackData_.pointCount = gv->GetInstance()->GetValueRef<int32_t>(name, "PointCount");
	attackData_.controlPoints.resize(attackData_.pointCount);
	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints[i] = gv->GetInstance()->GetValueRef<Vector3>(name, "ControlPoint_" + std::to_string(i));
	}

	// 移動系
	attackData_.moveSpeed = gv->GetInstance()->GetValueRef<float>(name, "MoveSpeed");
	attackData_.knockBackSpeed = gv->GetInstance()->GetValueRef<float>(name, "KnockBackSpeed");

	// タイマー系
	attackData_.totalDuration = gv->GetInstance()->GetValueRef<float>(name, "TotalDuration");
	attackData_.preDelay = gv->GetInstance()->GetValueRef<float>(name, "PreDelay");
	attackData_.attackDuration = gv->GetInstance()->GetValueRef<float>(name, "AttackDuration");
	attackData_.postDelay = gv->GetInstance()->GetValueRef<float>(name, "PostDelay");
	attackData_.nextAttackDelay = gv->GetInstance()->GetValueRef<float>(name, "NextAttackDelay");

	// その他
	attackData_.drawDebugControlPoints = gv->GetInstance()->GetValueRef<bool>(name, "DrawDebugControlPoints");
	attackData_.damage = gv->GetInstance()->GetValueRef<float>(name, "Damage");
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