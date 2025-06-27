#include "PlayerStateAttackBase.h"
#include <GlobalVariables.h>
#include <Primitive/PrimitiveDrawer.h>
#include <function.h>

PlayerStateAttackBase::PlayerStateAttackBase(std::string attackName)
{
	name = attackName;

	// 攻撃のデータを生成
	gv->CreateGroup(name);

	gv->AddItem(name, "PointCount", 0);



	gv->AddItem(name, "Damage", float());

	
}

void PlayerStateAttackBase::UpdateControlPoints()
{
	attackData_.pointCount = gv->GetInstance()->GetValueRef<int32_t>(name, "PointCount");
	attackData_.controlPoints.resize(attackData_.pointCount);
	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints[i] = gv->GetInstance()->GetValueRef<Vector3>(name, "ControlPoint_" + std::to_string(i));
	}
}

void PlayerStateAttackBase::DrawControlPoints()
{
	if (attackData_.pointCount < 4) return;

	std::vector<Vector3> curvePoints;
	for (int32_t i = 0; i < attackData_.pointCount - 3; ++i) {
		for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
			Vector3 point = CatmullRom(
				attackData_.controlPoints[i],
				attackData_.controlPoints[i + 1],
				attackData_.controlPoints[i + 2],
				attackData_.controlPoints[i + 3],
				t
			);
			curvePoints.push_back(point);
		}
	}
	for (size_t i = 0; i < curvePoints.size() - 1; i++) {
		PrimitiveDrawer::GetInstance()->DrawLine(curvePoints[i], curvePoints[i + 1], { 1.0f, 1.0f, 1.0f, 1.0f });
	}

}