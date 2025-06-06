#include "PlayerAttackEffect.h"
#include <Renderer/PrimitiveRenderer.h>

PlayerAttackEffect::PlayerAttackEffect()
{


}

void PlayerAttackEffect::Initialize()
{
    Object3d::Initialize();
    GetWorldTransform()->GetTranslation().y = 0.5f;
    GetRenderer("Cylinder1")->GetWorldTransform()->GetScale() = { 0.0f, 0.0f, 0.0f };
    GetRenderer("Cylinder2")->GetWorldTransform()->GetScale() = { 0.0f, 0.0f, 0.0f };
    GetRenderer("Cylinder3")->GetWorldTransform()->GetScale() = { 0.0f, 0.0f, 0.0f };
    GetRenderer("Cylinder4")->GetWorldTransform()->GetScale() = { 0.0f, 0.0f, 0.0f };

    Object3d::Update();
}

void PlayerAttackEffect::InitializeAttackCylinderEffect(float time)
{
	// 初期姿勢・スケール
	GetRenderer("Cylinder1")->GetWorldTransform()->GetRotation() = EulerDegree(Vector3{ 20.0f, 0.0f, 10.0f });
	GetRenderer("Cylinder2")->GetWorldTransform()->GetRotation() = EulerDegree(Vector3{ -20.0f, 0.0f, 10.0f });

	GetRenderer("Cylinder1")->GetWorldTransform()->GetScale() = { 3.0f, 0.0f, 3.0f };
	GetRenderer("Cylinder2")->GetWorldTransform()->GetScale() = { 3.0f, 0.0f, 3.0f };

	static_cast<Model*>(GetRenderer("Cylinder1")->GetModel())->GetMaterials(0)->GetUVData().position.x = 0.0f;
	static_cast<Model*>(GetRenderer("Cylinder2")->GetModel())->GetMaterials(0)->GetUVData().position.x = 0.0f;

	attackCylinderData_.isTrue = true;
	attackCylinderData_.timeData.max = time;
	attackCylinderData_.timeData.current = 0.0f;
	attackCylinderData_.phase = CylinderPhase::Appear;
	attackCylinderData_.elapsed = 0.0f;
	attackCylinderData_.scaleY = 0.0f;
}

void PlayerAttackEffect::InitializeTargetMarkerEffect(float time)
{
    GetRenderer("Cylinder3")->GetWorldTransform()->GetScale() = { 0.0f, 1.0f, 0.0f };
    GetRenderer("Cylinder4")->GetWorldTransform()->GetScale() = { 0.0f, 2.0f, 0.0f };

    static_cast<Model*>(GetRenderer("Cylinder3")->GetModel())->GetMaterials(0)->GetUVData().position.x = 0.0f;
    static_cast<Model*>(GetRenderer("Cylinder4")->GetModel())->GetMaterials(0)->GetUVData().position.x = 0.0f;
    static_cast<Model*>(GetRenderer("Cylinder3")->GetModel())->GetMaterials(0)->GetUVData().size = { 2.0f, 2.0f };
    static_cast<Model*>(GetRenderer("Cylinder4")->GetModel())->GetMaterials(0)->GetUVData().size = { 1.0f, 1.0f };
    static_cast<Model*>(GetRenderer("Cylinder3")->GetModel())->GetMaterials(0)->GetColor() = { 1.0f, 0.0f, 0.0f, 1.0f };
    static_cast<Model*>(GetRenderer("Cylinder4")->GetModel())->GetMaterials(0)->GetColor() = { 1.0f, 1.0f, 1.0f, 1.0f };

    targetMarkerData_.isTrue = true;
    targetMarkerData_.phase = MarkerPhase::Expand;
    targetMarkerData_.currentTime = 0.0f;

    // 省略可能（参考用）: time引数は合計時間を渡してきてる想定なら分解してもOK
    targetMarkerData_.timeData.max = time;
    targetMarkerData_.timeData.current = 0.0f;
}

void PlayerAttackEffect::Update()
{

}

void PlayerAttackEffect::UpdateAttackCylinderEffect(const Vector3& translate)
{
    if (!attackCylinderData_.isTrue) return;

    float deltaTime = 1.0f / 60.0f;
    attackCylinderData_.elapsed += deltaTime;

    float targetScaleY = 0.8f;

    switch (attackCylinderData_.phase)
    {
    case CylinderPhase::Appear:
    {
        float t = attackCylinderData_.elapsed / attackCylinderData_.appearDuration;
        if (t >= 1.0f) {
            t = 1.0f;
            attackCylinderData_.elapsed = 0.0f;
            attackCylinderData_.phase = CylinderPhase::Hold;
        }
        attackCylinderData_.scaleY = targetScaleY * t;
        break;
    }
    case CylinderPhase::Hold:
    {
        if (attackCylinderData_.elapsed >= attackCylinderData_.holdDuration) {
            attackCylinderData_.elapsed = 0.0f;
            attackCylinderData_.phase = CylinderPhase::Disappear;
        }
        attackCylinderData_.scaleY = targetScaleY;
        break;
    }
    case CylinderPhase::Disappear:
    {
        float t = attackCylinderData_.elapsed / attackCylinderData_.disappearDuration;
        if (t >= 1.0f) {
            t = 1.0f;
            attackCylinderData_.isTrue = false;
            attackCylinderData_.phase = CylinderPhase::Inactive;
            attackCylinderData_.scaleY = 0.0f;
        } else {
            attackCylinderData_.scaleY = targetScaleY * (1.0f - t);
        }
        break;
    }
    case CylinderPhase::Inactive:
        return;
    }

    // UV回転
    static_cast<Model*>(GetRenderer("Cylinder1")->GetModel())->GetMaterials(0)->GetUVData().position.x += attackCylinderData_.rotateSpeed;
    static_cast<Model*>(GetRenderer("Cylinder2")->GetModel())->GetMaterials(0)->GetUVData().position.x += attackCylinderData_.rotateSpeed;

    // 位置とスケールの更新
    Vector3 scale = { 3.0f, attackCylinderData_.scaleY, 3.0f };

    GetRenderer("Cylinder1")->GetWorldTransform()->GetTranslation() = translate;
    GetRenderer("Cylinder2")->GetWorldTransform()->GetTranslation() = translate;

    GetRenderer("Cylinder1")->GetWorldTransform()->GetTranslation().y += 0.5f;
    GetRenderer("Cylinder2")->GetWorldTransform()->GetTranslation().y += 0.5f;

    GetRenderer("Cylinder1")->GetWorldTransform()->GetScale() = scale;
    GetRenderer("Cylinder2")->GetWorldTransform()->GetScale() = scale;

	Object3d::Update();
}

void PlayerAttackEffect::UpdateTargetMarkerEffect(const Vector3& translate)
{
    if (!targetMarkerData_.isTrue) return;

    // UV回転
    static_cast<Model*>(GetRenderer("Cylinder3")->GetModel())->GetMaterials(0)->GetUVData().position.x += targetMarkerData_.rotateSpeed;
    static_cast<Model*>(GetRenderer("Cylinder4")->GetModel())->GetMaterials(0)->GetUVData().position.x += targetMarkerData_.rotateSpeed;

    GetRenderer("Cylinder3")->GetWorldTransform()->GetTranslation() = translate;
    GetRenderer("Cylinder4")->GetWorldTransform()->GetTranslation() = translate;

    // 時間進行
    targetMarkerData_.currentTime += 1.0f / 60.0f;

    float t = 0.0f;
    switch (targetMarkerData_.phase)
    {
    case MarkerPhase::Expand:
        t = std::clamp(targetMarkerData_.currentTime / targetMarkerData_.expandDuration, 0.0f, 1.0f);
        {
            float scaleX = std::lerp(0.0f, 1.5f, t);
            float scaleZ = std::lerp(0.0f, 1.5f, t);
            GetRenderer("Cylinder3")->GetWorldTransform()->GetScale() = { scaleX, 1.0f, scaleZ };

            scaleX = std::lerp(0.0f, 2.0f, t);
            scaleZ = std::lerp(0.0f, 2.0f, t);
            GetRenderer("Cylinder4")->GetWorldTransform()->GetScale() = { scaleX, 2.0f, scaleZ };
        }

        if (targetMarkerData_.currentTime >= targetMarkerData_.expandDuration) {
            targetMarkerData_.phase = MarkerPhase::Wait;
            targetMarkerData_.currentTime = 0.0f;
        }
        break;

    case MarkerPhase::Wait:
        // スケールを維持（省略せず明示的に保持）
        GetRenderer("Cylinder3")->GetWorldTransform()->GetScale() = { 1.5f, 1.0f, 1.5f };
        GetRenderer("Cylinder4")->GetWorldTransform()->GetScale() = { 2.0f, 2.0f, 2.0f };

        if (targetMarkerData_.currentTime >= targetMarkerData_.waitDuration) {
            targetMarkerData_.phase = MarkerPhase::Shrink;
            targetMarkerData_.currentTime = 0.0f;
        }
        break;

    case MarkerPhase::Shrink:
        t = std::clamp(targetMarkerData_.currentTime / targetMarkerData_.shrinkDuration, 0.0f, 1.0f);
        {
            float scaleX = std::lerp(1.5f, 0.0f, t);
            float scaleZ = std::lerp(1.5f, 0.0f, t);
            GetRenderer("Cylinder3")->GetWorldTransform()->GetScale() = { scaleX, 1.0f, scaleZ };

            scaleX = std::lerp(2.0f, 0.0f, t);
            scaleZ = std::lerp(2.0f, 0.0f, t);
            GetRenderer("Cylinder4")->GetWorldTransform()->GetScale() = { scaleX, 2.0f, scaleZ };
        }

        if (targetMarkerData_.currentTime >= targetMarkerData_.shrinkDuration) {
            targetMarkerData_.phase = MarkerPhase::End;
            targetMarkerData_.isTrue = false;
            //InitializeTargetMarkerEffect(100.0f);
        }
        break;

    case MarkerPhase::End:
        // 表示終了、何もしない
        break;
    }

    Object3d::Update();
}

void PlayerAttackEffect::Draw()
{
	Object3d::Draw();
}

#ifdef _DEBUG
void PlayerAttackEffect::DebugGui()
{
	Object3d::DebugGui();
}
#endif // _DEBUG