#include "BossVfxUtil.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Camera/GameCamera.h"
#include "Platform/WindowManager.h"
#include "World3D/Camera/CameraManager.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "World3D/Object/Model/Animation/SkinnedInstance.h"
#include "World3D/Object/Model/Animation/Skeleton.h"

namespace BossVfxUtil {

	bool TryGetJointWorldPosition(Enemy& enemy, const std::string& rendererName,
		const char* jointName, Vector3& outPosition)
	{
		BaseRenderer* renderer = enemy.GetRenderer(rendererName);
		if (!renderer) return false;

		SkinnedInstance* instance = renderer->GetSkinnedInstance();
		if (!instance) return false;

		const Joint* joint = instance->GetSkeleton()->FindJoint(jointName);
		if (!joint) return false;

		// ジョイントのスケルトン空間行列 × レンダラーのワールド行列（BoneAttachment と同じ式）。
		// 行ベクトル規約なので、出来上がった行列の4行目がワールド座標
		const Matrix4x4 world = joint->skeletonSpaceMatrix * renderer->GetWorldTransform()->GetMatWorld();
		outPosition = Vector3{ world.m[3][0], world.m[3][1], world.m[3][2] };
		return true;
	}

	Vector3 GetJointWorldPositionOr(Enemy& enemy, const std::string& rendererName,
		const char* jointName, const Vector3& fallback)
	{
		Vector3 position{};
		return TryGetJointWorldPosition(enemy, rendererName, jointName, position) ? position : fallback;
	}

	bool ToScreenUV(const Vector3& worldPosition, Vector2& outUV)
	{
		BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
		if (!camera || !camera->IsInView(worldPosition)) return false;

		const Vector2 screen = camera->WorldToScreen(worldPosition,
			static_cast<int>(WindowManager::kGameWidth), static_cast<int>(WindowManager::kGameHeight));
		outUV = {
			screen.x / static_cast<float>(WindowManager::kGameWidth),
			screen.y / static_cast<float>(WindowManager::kGameHeight)
		};
		return true;
	}

	bool ProjectToScreen(const Vector3& worldPosition, float worldRadius, Vector2& outUV, float& outRadius)
	{
		BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
		if (!camera) return false;

		// 行ベクトル規約（v * VP）。画面の外でも、カメラの前にあれば帯の端として使えるので IsInView は見ない
		const Matrix4x4& vp = camera->GetViewProjectionMatrix();
		const Vector3& p = worldPosition;
		const float clipX = p.x * vp.m[0][0] + p.y * vp.m[1][0] + p.z * vp.m[2][0] + vp.m[3][0];
		const float clipY = p.x * vp.m[0][1] + p.y * vp.m[1][1] + p.z * vp.m[2][1] + vp.m[3][1];
		const float clipW = p.x * vp.m[0][3] + p.y * vp.m[1][3] + p.z * vp.m[2][3] + vp.m[3][3];
		if (clipW <= 0.05f) return false;

		outUV = { clipX / clipW * 0.5f + 0.5f, -clipY / clipW * 0.5f + 0.5f };
		// 射影行列の [1][1] は 1/tan(縦画角/2)。NDC の縦は2なので、UV（縦=1）では半分になる
		outRadius = worldRadius * camera->GetProjectionMatrix().m[1][1] * 0.5f / clipW;
		return true;
	}

	void AddCameraShake(float trauma)
	{
		if (trauma <= 0.0f) return;
		if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
			camera->AddShake(trauma);
		}
	}

	void AddCameraFovPunch(float add)
	{
		if (add <= 0.0f) return;
		if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
			camera->AddFovPunch(add);
		}
	}

} // namespace BossVfxUtil
