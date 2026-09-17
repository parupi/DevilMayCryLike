#include "EditorViewMath.h"
#ifdef _DEBUG

#include "EditorContext.h"

#include "Math/Vector4.h"
#include "World3D/Camera/BaseCamera.h"
#include "World3D/Camera/CameraManager.h"

#include <cmath>

namespace {

const Vector3 kAxisX{ 1.0f, 0.0f, 0.0f };
const Vector3 kAxisY{ 0.0f, 1.0f, 0.0f };
const Vector3 kAxisZ{ 0.0f, 0.0f, 1.0f };

} // namespace

Vector3 EditorView::SafeNormalize(const Vector3& v, const Vector3& fallback)
{
	const float length = Length(v);
	return (length > 1e-6f) ? Vector3{ v.x / length, v.y / length, v.z / length } : fallback;
}

bool EditorView::Build(Context& out, const ImVec2& imagePos, const ImVec2& imageSize)
{
	if (imageSize.x <= 1.0f || imageSize.y <= 1.0f) {
		return false;
	}

	CameraManager* cameraManager = Editor::Ctx().cameraManager;
	BaseCamera* camera = cameraManager ? cameraManager->GetActiveCamera() : nullptr;
	if (!camera) {
		return false;
	}

	out.viewProjection = camera->GetViewProjectionMatrix();
	out.inverseViewProjection = Inverse(out.viewProjection);
	// Inverse() は行列式のゼロ割りを見ていない。潰れた行列だと NaN が伝播するので弾く
	if (!std::isfinite(out.inverseViewProjection.m[0][0])) {
		return false;
	}

	// カメラのワールド行列は行ベクトル。0行目が右、1行目が上、2行目が前、3行目が位置
	const Matrix4x4& cameraWorld = camera->GetWorldMatrix();
	out.cameraRight = SafeNormalize({ cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2] }, kAxisX);
	out.cameraUp = SafeNormalize({ cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2] }, kAxisY);
	out.cameraForward = SafeNormalize({ cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2] }, kAxisZ);
	out.cameraPosition = { cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2] };

	out.imagePos = imagePos;
	out.imageSize = imageSize;
	return true;
}

bool EditorView::WorldToScreen(const Context& view, const Vector3& world, ImVec2& outScreen)
{
	const Vector4 clip = Vector4{ world.x, world.y, world.z, 1.0f } * view.viewProjection;
	if (clip.w <= 1e-4f) {
		return false;
	}
	const float ndcX = clip.x / clip.w;
	const float ndcY = clip.y / clip.w;
	outScreen.x = view.imagePos.x + (ndcX * 0.5f + 0.5f) * view.imageSize.x;
	outScreen.y = view.imagePos.y + (0.5f - ndcY * 0.5f) * view.imageSize.y;
	return true;
}

void EditorView::ScreenToRay(const Context& view, const ImVec2& screen,
	Vector3& outOrigin, Vector3& outDirection)
{
	const float ndcX = ((screen.x - view.imagePos.x) / view.imageSize.x) * 2.0f - 1.0f;
	const float ndcY = 1.0f - ((screen.y - view.imagePos.y) / view.imageSize.y) * 2.0f;

	// D3D の深度は [0,1]。ニア面とファー面の2点を戻して結ぶ
	const auto unproject = [&](float ndcZ) {
		const Vector4 p = Vector4{ ndcX, ndcY, ndcZ, 1.0f } * view.inverseViewProjection;
		const float invW = (std::abs(p.w) > 1e-8f) ? (1.0f / p.w) : 0.0f;
		return Vector3{ p.x * invW, p.y * invW, p.z * invW };
	};

	outOrigin = unproject(0.0f);
	outDirection = SafeNormalize(unproject(1.0f) - outOrigin, view.cameraForward);
}

bool EditorView::IsInsideImage(const Context& view, const ImVec2& screen)
{
	return screen.x >= view.imagePos.x && screen.x <= view.imagePos.x + view.imageSize.x
		&& screen.y >= view.imagePos.y && screen.y <= view.imagePos.y + view.imageSize.y;
}

#endif // _DEBUG
