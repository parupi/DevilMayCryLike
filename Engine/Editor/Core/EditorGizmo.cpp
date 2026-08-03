#include "EditorGizmo.h"
#ifdef _DEBUG

#include "EditorContext.h"
#include "EditorSelection.h"

#include "Debugger/GlobalVariables.h"
#include "Math/MathUtils.h"
#include "Math/Vector4.h"
#include "World3D/Camera/BaseCamera.h"
#include "World3D/Camera/CameraManager.h"
#include "World3D/Object/Object3d.h"
#include "World3D/WorldTransform.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <numbers>

namespace {

using Operation = EditorGizmo::Operation;
using Space = EditorGizmo::Space;

constexpr const char* kDirectoryName = "Editor";
constexpr const char* kGroupName = "EditorGizmo";

constexpr float kPi = std::numbers::pi_v<float>;

// --- 見た目（断りがなければピクセル） ---
constexpr float kGizmoPixels     = 90.0f;  // 軸の長さ。ここを基準に全部の大きさが決まる
constexpr float kAxisGrab        = 8.0f;   // 軸を掴める距離
constexpr float kRingGrab        = 7.0f;   // 回転リングを掴める距離
constexpr float kCenterPixels    = 8.0f;   // 中央ハンドルの半径
constexpr float kMinAxisPixels   = 14.0f;  // 視線方向に潰れた軸はこれ以下で隠す
constexpr float kArrowLength     = 14.0f;
constexpr float kArrowWidth      = 5.0f;
constexpr float kScaleBoxHalf    = 5.0f;
constexpr float kPlaneOffset     = 0.32f;  // 平面ハンドルの位置（軸長に対する比）
constexpr float kPlaneSize       = 0.26f;  // 平面ハンドルの一辺（軸長に対する比）
constexpr float kScreenRingScale = 1.22f;  // 画面回転リングの半径（軸長に対する比）
constexpr float kPlaneFacingMin  = 0.20f;  // 真横を向いた平面ハンドルは隠す

const ImU32 kAxisColor[3] = {
	IM_COL32(226, 74, 74, 255),
	IM_COL32(120, 205, 60, 255),
	IM_COL32(74, 134, 240, 255),
};
const ImU32 kAxisFill[3] = {
	IM_COL32(226, 74, 74, 70),
	IM_COL32(120, 205, 60, 70),
	IM_COL32(74, 134, 240, 70),
};
constexpr ImU32 kHighlight = IM_COL32(255, 205, 60, 255);
constexpr ImU32 kHighlightFill = IM_COL32(255, 205, 60, 110);
constexpr ImU32 kScreenColor = IM_COL32(215, 215, 225, 220);
constexpr ImU32 kReadoutBg = IM_COL32(20, 20, 24, 200);
constexpr ImU32 kReadoutText = IM_COL32(240, 240, 245, 255);

const Vector3 kWorldAxis[3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
const char* const kAxisName[3] = { "X", "Y", "Z" };

// ハンドルの番号。ドラッグ中どれを掴んでいるかもこれで持つ
constexpr int kHandleNone = -1;
constexpr int kHandleAxis = 0;   // +0..+2 … 軸そのもの
constexpr int kHandlePlane = 3;  // +0..+2 … 法線が軸 i の平面
constexpr int kHandleScreen = 6; // 画面平面の移動 / 画面回転 / 均一拡縮

// --- 設定（EditorGizmo.json に保存される） ---

bool g_enabled = true;
Operation g_operation = Operation::Translate;
Space g_space = Space::World;
bool g_showToolbar = true;
bool g_useUnityKeys = false; // W/E/R でも切り替えるか（ゲーム入力と衝突するので既定はオフ）

bool g_snapTranslate = false;
float g_snapTranslateValue = 1.0f;
bool g_snapRotate = false;
float g_snapRotateDegree = 15.0f;
bool g_snapScale = false;
float g_snapScaleValue = 0.1f;

// --- 実行時の状態 ---

bool g_isOver = false;
// ドラッグ中に Ctrl を押している間だけ true。スナップの有無を一時的に反転させる
bool g_snapOverride = false;

/// <summary>
/// ドラッグ中に固定しておく情報。
/// 軸・原点・掴んだ位置はドラッグ開始時のものを使い続ける。
/// 毎フレーム引き直すと、動かした結果が次の計算に混ざって暴走するため。
/// </summary>
struct DragState {
	bool active = false;
	int handle = kHandleNone;
	Operation operation = Operation::Translate;

	// 開始時のローカル値
	Vector3 startTranslation{};
	Quaternion startRotation{};
	Vector3 startScale{ 1.0f, 1.0f, 1.0f };

	// 開始時に固定した軸と原点（ワールド）
	Vector3 origin{};
	Vector3 axes[3] = { kWorldAxis[0], kWorldAxis[1], kWorldAxis[2] };
	bool hasParent = false;
	Matrix4x4 invParentWorld;

	// 移動
	float startAxisT = 0.0f;
	Vector3 startHit{};
	Vector3 planeNormal{};
	Vector3 planeU{};
	Vector3 planeV{};

	// 回転
	Vector3 rotAxis{};
	Vector3 rotU{};
	Vector3 rotV{};
	float lastAngle = 0.0f;
	float accumAngle = 0.0f;
	float appliedAngle = 0.0f;

	// 拡縮
	ImVec2 startOriginScreen{};
	ImVec2 axisScreenDir{};
	float axisScreenLen = 1.0f;
	float startProjection = 0.0f;
	float startRadius = 0.0f;
};

DragState g_drag;

// --- 小物 ---

inline ImVec2 Add2(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); }
inline ImVec2 Sub2(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x - b.x, a.y - b.y); }
inline ImVec2 Mul2(const ImVec2& a, float s) { return ImVec2(a.x * s, a.y * s); }
inline float Dot2(const ImVec2& a, const ImVec2& b) { return a.x * b.x + a.y * b.y; }
inline float Len2(const ImVec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }

ImVec2 Norm2(const ImVec2& v)
{
	const float length = Len2(v);
	return (length > 1e-5f) ? ImVec2(v.x / length, v.y / length) : ImVec2(1.0f, 0.0f);
}

Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback)
{
	const float length = Length(v);
	return (length > 1e-6f) ? Vector3{ v.x / length, v.y / length, v.z / length } : fallback;
}

Quaternion SafeNormalizeQuat(const Quaternion& q)
{
	const float norm = Norm(q);
	return (norm > 1e-6f) ? Quaternion(q.x / norm, q.y / norm, q.z / norm, q.w / norm) : Identity();
}

float SnapTo(float value, float step)
{
	return (step > 1e-5f) ? std::round(value / step) * step : value;
}

/// <summary>その操作で今スナップが効いているか（ドラッグ中の Ctrl で反転する）</summary>
bool SnapActive(Operation operation)
{
	const bool base = (operation == Operation::Translate) ? g_snapTranslate
		: (operation == Operation::Rotate) ? g_snapRotate
		: g_snapScale;
	return g_snapOverride ? !base : base;
}

/// <summary>axis に垂直な単位ベクトルを1本作る</summary>
Vector3 AnyPerpendicular(const Vector3& axis)
{
	const Vector3 reference = (std::abs(axis.y) < 0.9f) ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
	return SafeNormalize(Cross(reference, axis), Vector3{ 1.0f, 0.0f, 0.0f });
}

/// <summary>画像の中のどこにギズモを描くかを決めるための、カメラと表示領域の情報</summary>
struct ViewContext {
	Matrix4x4 viewProjection;
	Matrix4x4 inverseViewProjection;
	Vector3 cameraPosition{};
	Vector3 cameraRight{};
	Vector3 cameraUp{};
	Vector3 cameraForward{};
	ImVec2 imagePos{};
	ImVec2 imageSize{};
};

/// <summary>ワールド座標を画像上のスクリーン座標へ。カメラの後ろなら false</summary>
bool WorldToScreen(const ViewContext& view, const Vector3& world, ImVec2& outScreen)
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

/// <summary>画像上のスクリーン座標からワールドのレイを作る</summary>
void ScreenToRay(const ViewContext& view, const ImVec2& screen, Vector3& outOrigin, Vector3& outDirection)
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

/// <summary>ギズモの実寸（ワールド）。画面上でいつも同じ大きさに見えるようにする</summary>
float ComputeGizmoWorldSize(const ViewContext& view, const Vector3& origin)
{
	ImVec2 a{};
	ImVec2 b{};
	if (!WorldToScreen(view, origin, a) || !WorldToScreen(view, origin + view.cameraRight, b)) {
		return 0.0f;
	}
	const float pixelsPerUnit = Len2(Sub2(b, a));
	return (pixelsPerUnit > 1e-3f) ? (kGizmoPixels / pixelsPerUnit) : 0.0f;
}

float DistanceToSegment(const ImVec2& point, const ImVec2& a, const ImVec2& b)
{
	const ImVec2 ab = Sub2(b, a);
	const float lengthSq = Dot2(ab, ab);
	if (lengthSq < 1e-6f) {
		return Len2(Sub2(point, a));
	}
	float t = Dot2(Sub2(point, a), ab) / lengthSq;
	t = (std::max)(0.0f, (std::min)(1.0f, t));
	return Len2(Sub2(point, Add2(a, Mul2(ab, t))));
}

/// <summary>凸四角形の内側か（頂点は一周する順で渡すこと）</summary>
bool PointInQuad(const ImVec2& point, const ImVec2 quad[4])
{
	bool positive = false;
	bool negative = false;
	for (int i = 0; i < 4; ++i) {
		const ImVec2 edge = Sub2(quad[(i + 1) % 4], quad[i]);
		const ImVec2 toPoint = Sub2(point, quad[i]);
		const float cross = edge.x * toPoint.y - edge.y * toPoint.x;
		if (cross > 0.0f) positive = true;
		if (cross < 0.0f) negative = true;
	}
	return !(positive && negative);
}

/// <summary>レイと平面の交点</summary>
bool RayPlane(const Vector3& rayOrigin, const Vector3& rayDirection,
	const Vector3& planePoint, const Vector3& planeNormal, Vector3& outPoint)
{
	const float denominator = Dot(rayDirection, planeNormal);
	if (std::abs(denominator) < 1e-5f) {
		return false;
	}
	const float t = Dot(planePoint - rayOrigin, planeNormal) / denominator;
	if (t <= 0.0f) {
		return false;
	}
	outPoint = rayOrigin + rayDirection * t;
	return true;
}

/// <summary>
/// 直線 origin + t * axis のうち、レイに最も近い点のパラメータ t を求める。
/// 軸を真正面から覗き込んでいる（軸とレイが平行な）ときだけ false。
/// </summary>
bool ClosestPointOnAxis(const Vector3& origin, const Vector3& axis,
	const Vector3& rayOrigin, const Vector3& rayDirection, float& outT)
{
	const Vector3 w0 = origin - rayOrigin;
	const float b = Dot(axis, rayDirection);
	const float d = Dot(axis, w0);
	const float e = Dot(rayDirection, w0);
	const float denominator = 1.0f - b * b; // axis も rayDirection も正規化済み
	if (std::abs(denominator) < 1e-5f) {
		return false;
	}
	outT = (b * e - d) / denominator;
	return true;
}

/// <summary>ギズモ1回ぶんの計算結果。ピック・描画で使い回す</summary>
struct Frame {
	ViewContext view;
	Vector3 origin{};
	Vector3 axes[3] = { kWorldAxis[0], kWorldAxis[1], kWorldAxis[2] };
	float size = 1.0f;
	ImVec2 originScreen{};
	Vector3 rayOrigin{};
	Vector3 rayDirection{};
	ImVec2 mouse{};
	bool canInteract = false;
	ImDrawList* drawList = nullptr;
};

// --- 移動ギズモ ---

/// <summary>平面ハンドルの4隅。カメラ側に来るように符号を反転させる</summary>
void BuildPlaneCorners(const Frame& frame, int normalAxis, Vector3 outCorners[4])
{
	const int j = (normalAxis + 1) % 3;
	const int k = (normalAxis + 2) % 3;
	const Vector3 toCamera = frame.view.cameraPosition - frame.origin;

	const Vector3 aj = frame.axes[j] * ((Dot(toCamera, frame.axes[j]) >= 0.0f) ? 1.0f : -1.0f);
	const Vector3 ak = frame.axes[k] * ((Dot(toCamera, frame.axes[k]) >= 0.0f) ? 1.0f : -1.0f);

	const float offset = frame.size * kPlaneOffset;
	const float side = frame.size * kPlaneSize;

	outCorners[0] = frame.origin + aj * offset + ak * offset;
	outCorners[1] = frame.origin + aj * (offset + side) + ak * offset;
	outCorners[2] = frame.origin + aj * (offset + side) + ak * (offset + side);
	outCorners[3] = frame.origin + aj * offset + ak * (offset + side);
}

bool IsPlaneVisible(const Frame& frame, int normalAxis)
{
	const Vector3 toCamera = SafeNormalize(frame.view.cameraPosition - frame.origin, frame.view.cameraForward);
	return std::abs(Dot(toCamera, frame.axes[normalAxis])) > kPlaneFacingMin;
}

int PickAndDrawTranslate(const Frame& frame, int activeHandle)
{
	ImVec2 axisEnd[3]{};
	bool axisVisible[3]{};
	for (int i = 0; i < 3; ++i) {
		axisVisible[i] = WorldToScreen(frame.view, frame.origin + frame.axes[i] * frame.size, axisEnd[i])
			&& Len2(Sub2(axisEnd[i], frame.originScreen)) > kMinAxisPixels;
	}

	ImVec2 planeQuad[3][4]{};
	bool planeVisible[3]{};
	for (int i = 0; i < 3; ++i) {
		planeVisible[i] = IsPlaneVisible(frame, i);
		if (!planeVisible[i]) {
			continue;
		}
		Vector3 corners[4];
		BuildPlaneCorners(frame, i, corners);
		for (int c = 0; c < 4; ++c) {
			if (!WorldToScreen(frame.view, corners[c], planeQuad[i][c])) {
				planeVisible[i] = false;
				break;
			}
		}
	}

	// 中央 → 平面 → 軸 の順に拾う。手前にある（＝細かい）ものを優先する
	int hovered = kHandleNone;
	if (frame.canInteract) {
		if (Len2(Sub2(frame.mouse, frame.originScreen)) <= kCenterPixels + 2.0f) {
			hovered = kHandleScreen;
		}
		for (int i = 0; hovered == kHandleNone && i < 3; ++i) {
			if (planeVisible[i] && PointInQuad(frame.mouse, planeQuad[i])) {
				hovered = kHandlePlane + i;
			}
		}
		// 軸どうしが重なっている場所では、一番近い軸を選ぶ
		if (hovered == kHandleNone) {
			float bestDistance = kAxisGrab;
			for (int i = 0; i < 3; ++i) {
				if (!axisVisible[i]) {
					continue;
				}
				const float distance = DistanceToSegment(frame.mouse, frame.originScreen, axisEnd[i]);
				if (distance < bestDistance) {
					bestDistance = distance;
					hovered = kHandleAxis + i;
				}
			}
		}
	}

	const int emphasized = (activeHandle != kHandleNone) ? activeHandle : hovered;

	// 平面 → 軸 → 中央 の順に描く（重なったとき手前に来てほしいものを後に）
	for (int i = 0; i < 3; ++i) {
		if (!planeVisible[i]) {
			continue;
		}
		const bool lit = (emphasized == kHandlePlane + i);
		frame.drawList->AddQuadFilled(planeQuad[i][0], planeQuad[i][1], planeQuad[i][2], planeQuad[i][3],
			lit ? kHighlightFill : kAxisFill[i]);
		frame.drawList->AddQuad(planeQuad[i][0], planeQuad[i][1], planeQuad[i][2], planeQuad[i][3],
			lit ? kHighlight : kAxisColor[i], 1.5f);
	}

	for (int i = 0; i < 3; ++i) {
		if (!axisVisible[i]) {
			continue;
		}
		const ImU32 color = (emphasized == kHandleAxis + i) ? kHighlight : kAxisColor[i];
		const ImVec2 direction = Norm2(Sub2(axisEnd[i], frame.originScreen));
		const ImVec2 normal(-direction.y, direction.x);

		frame.drawList->AddLine(frame.originScreen, axisEnd[i], color, 2.5f);
		frame.drawList->AddTriangleFilled(axisEnd[i],
			Add2(Sub2(axisEnd[i], Mul2(direction, kArrowLength)), Mul2(normal, kArrowWidth)),
			Sub2(Sub2(axisEnd[i], Mul2(direction, kArrowLength)), Mul2(normal, kArrowWidth)),
			color);
	}

	const ImU32 centerColor = (emphasized == kHandleScreen) ? kHighlight : kScreenColor;
	frame.drawList->AddCircleFilled(frame.originScreen, kCenterPixels * 0.55f, centerColor);
	frame.drawList->AddCircle(frame.originScreen, kCenterPixels, centerColor, 0, 1.5f);

	return hovered;
}

// --- 回転ギズモ ---

constexpr int kRingSegments = 64;

/// <summary>色はそのままにアルファだけ弱める</summary>
ImU32 Fade(ImU32 color, float alphaScale)
{
	const ImU32 alpha = static_cast<ImU32>(((color >> IM_COL32_A_SHIFT) & 0xFF) * alphaScale);
	return (color & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
}

/// <summary>
/// 回転リングを描く／マウスとの最短距離を測る。
/// 奥へ回り込む半周は薄くする。消してしまうと、カメラの正面を向いたリングが
/// 「たまたま半分だけ残る」状態になって、輪に見えなくなるため。
/// </summary>
/// <param name="draw">false なら距離を測るだけで描かない</param>
float TraceRotationRing(const Frame& frame, int axisIndex, ImU32 color, bool draw)
{
	const Vector3& axis = frame.axes[axisIndex];
	const Vector3 u = AnyPerpendicular(axis);
	const Vector3 v = Cross(axis, u);
	const Vector3 toCamera = frame.view.cameraPosition - frame.origin;
	const ImU32 backColor = Fade(color, 0.35f);

	float nearest = FLT_MAX;
	ImVec2 run[kRingSegments + 2];
	int runCount = 0;
	bool runIsFront = true;

	const auto flush = [&]() {
		if (runCount >= 2) {
			if (draw) {
				frame.drawList->AddPolyline(run, runCount,
					runIsFront ? color : backColor, ImDrawFlags_None, runIsFront ? 2.0f : 1.5f);
			} else {
				for (int i = 0; i + 1 < runCount; ++i) {
					nearest = (std::min)(nearest, DistanceToSegment(frame.mouse, run[i], run[i + 1]));
				}
			}
		}
		runCount = 0;
	};

	for (int s = 0; s <= kRingSegments; ++s) {
		const float angle = (2.0f * kPi * static_cast<float>(s)) / static_cast<float>(kRingSegments);
		const Vector3 offset = u * std::cos(angle) + v * std::sin(angle);

		ImVec2 screen{};
		if (!WorldToScreen(frame.view, frame.origin + offset * frame.size, screen)) {
			flush();
			continue;
		}

		// 手前と奥で線を分ける。境目の点は両方の run に入れて隙間を作らない
		const bool isFront = (Dot(offset, toCamera) > 0.0f);
		if (runCount > 0 && isFront != runIsFront) {
			run[runCount++] = screen;
			flush();
		}
		runIsFront = isFront;
		run[runCount++] = screen;
	}
	flush();

	return nearest;
}

/// <summary>ドラッグ開始位置から今までの回転量を扇形で見せる</summary>
void DrawRotationArc(const Frame& frame)
{
	constexpr int kArcSegments = 48;
	const float startAngle = g_drag.lastAngle - g_drag.accumAngle;

	ImVec2 arc[kArcSegments + 1];
	int count = 0;
	for (int s = 0; s <= kArcSegments; ++s) {
		const float t = static_cast<float>(s) / static_cast<float>(kArcSegments);
		const float angle = startAngle + g_drag.appliedAngle * t;
		const Vector3 offset = g_drag.rotU * std::cos(angle) + g_drag.rotV * std::sin(angle);
		ImVec2 screen{};
		if (!WorldToScreen(frame.view, g_drag.origin + offset * frame.size, screen)) {
			break;
		}
		arc[count++] = screen;
	}
	if (count < 2) {
		return;
	}

	// 半周を超えると凸でなくなるので、塗りではなく弧＋半径の線で見せる
	frame.drawList->AddPolyline(arc, count, IM_COL32(255, 205, 60, 200), ImDrawFlags_None, 3.0f);
	frame.drawList->AddLine(frame.originScreen, arc[0], IM_COL32(255, 205, 60, 140), 1.5f);
	frame.drawList->AddLine(frame.originScreen, arc[count - 1], IM_COL32(255, 205, 60, 200), 1.5f);
}

int PickAndDrawRotate(const Frame& frame, int activeHandle)
{
	const float screenRadius = kGizmoPixels * kScreenRingScale;

	// リングは重なるので、まず全部の距離を測って「一番近いものだけ」を光らせる
	int hovered = kHandleNone;
	if (frame.canInteract) {
		float best = kRingGrab;
		for (int i = 0; i < 3; ++i) {
			const float distance = TraceRotationRing(frame, i, 0, false);
			if (distance < best) {
				best = distance;
				hovered = kHandleAxis + i;
			}
		}
		const float screenDistance = std::abs(Len2(Sub2(frame.mouse, frame.originScreen)) - screenRadius);
		if (screenDistance < best) {
			hovered = kHandleScreen;
		}
	}

	const int emphasized = (activeHandle != kHandleNone) ? activeHandle : hovered;

	frame.drawList->AddCircle(frame.originScreen, screenRadius,
		(emphasized == kHandleScreen) ? kHighlight : kScreenColor, 0, 2.0f);

	for (int i = 0; i < 3; ++i) {
		TraceRotationRing(frame, i, (emphasized == kHandleAxis + i) ? kHighlight : kAxisColor[i], true);
	}

	if (g_drag.active && g_drag.operation == Operation::Rotate) {
		DrawRotationArc(frame);
	}

	frame.drawList->AddCircleFilled(frame.originScreen, 3.0f, kScreenColor);
	return hovered;
}

// --- 拡縮ギズモ ---

int PickAndDrawScale(const Frame& frame, int activeHandle)
{
	ImVec2 axisEnd[3]{};
	bool axisVisible[3]{};
	for (int i = 0; i < 3; ++i) {
		axisVisible[i] = WorldToScreen(frame.view, frame.origin + frame.axes[i] * frame.size, axisEnd[i])
			&& Len2(Sub2(axisEnd[i], frame.originScreen)) > kMinAxisPixels;
	}

	int hovered = kHandleNone;
	if (frame.canInteract) {
		if (Len2(Sub2(frame.mouse, frame.originScreen)) <= kCenterPixels + 2.0f) {
			hovered = kHandleScreen;
		}
		// 先端の箱を優先。外れたら軸の線でも拾う
		for (int i = 0; hovered == kHandleNone && i < 3; ++i) {
			if (axisVisible[i] && Len2(Sub2(frame.mouse, axisEnd[i])) <= kScaleBoxHalf + 3.0f) {
				hovered = kHandleAxis + i;
			}
		}
		if (hovered == kHandleNone) {
			float bestDistance = kAxisGrab;
			for (int i = 0; i < 3; ++i) {
				if (!axisVisible[i]) {
					continue;
				}
				const float distance = DistanceToSegment(frame.mouse, frame.originScreen, axisEnd[i]);
				if (distance < bestDistance) {
					bestDistance = distance;
					hovered = kHandleAxis + i;
				}
			}
		}
	}

	const int emphasized = (activeHandle != kHandleNone) ? activeHandle : hovered;

	for (int i = 0; i < 3; ++i) {
		if (!axisVisible[i]) {
			continue;
		}
		const ImU32 color = (emphasized == kHandleAxis + i) ? kHighlight : kAxisColor[i];
		frame.drawList->AddLine(frame.originScreen, axisEnd[i], color, 2.5f);
		frame.drawList->AddRectFilled(
			ImVec2(axisEnd[i].x - kScaleBoxHalf, axisEnd[i].y - kScaleBoxHalf),
			ImVec2(axisEnd[i].x + kScaleBoxHalf, axisEnd[i].y + kScaleBoxHalf), color);
	}

	const ImU32 centerColor = (emphasized == kHandleScreen) ? kHighlight : kScreenColor;
	frame.drawList->AddRectFilled(
		ImVec2(frame.originScreen.x - kCenterPixels * 0.6f, frame.originScreen.y - kCenterPixels * 0.6f),
		ImVec2(frame.originScreen.x + kCenterPixels * 0.6f, frame.originScreen.y + kCenterPixels * 0.6f),
		centerColor);
	frame.drawList->AddRect(
		ImVec2(frame.originScreen.x - kCenterPixels, frame.originScreen.y - kCenterPixels),
		ImVec2(frame.originScreen.x + kCenterPixels, frame.originScreen.y + kCenterPixels),
		centerColor, 0.0f, 0, 1.5f);

	return hovered;
}

// --- ドラッグ ---

void BeginDrag(const Frame& frame, WorldTransform* transform, int handle,
	bool hasParent, const Matrix4x4& parentWorld)
{
	g_drag = DragState{};
	g_drag.active = true;
	g_drag.handle = handle;
	g_drag.operation = g_operation;

	g_drag.startTranslation = transform->GetTranslation();
	g_drag.startRotation = SafeNormalizeQuat(transform->GetRotation());
	g_drag.startScale = transform->GetScale();

	g_drag.origin = frame.origin;
	for (int i = 0; i < 3; ++i) {
		g_drag.axes[i] = frame.axes[i];
	}
	g_drag.hasParent = hasParent;
	g_drag.invParentWorld = hasParent ? Inverse(parentWorld) : MakeIdentity4x4();

	g_drag.startOriginScreen = frame.originScreen;

	switch (g_operation) {
	case Operation::Translate:
		if (handle >= kHandleAxis && handle < kHandleAxis + 3) {
			if (!ClosestPointOnAxis(frame.origin, frame.axes[handle - kHandleAxis],
				frame.rayOrigin, frame.rayDirection, g_drag.startAxisT)) {
				g_drag.active = false;
			}
		} else if (handle >= kHandlePlane && handle < kHandlePlane + 3) {
			const int i = handle - kHandlePlane;
			g_drag.planeNormal = frame.axes[i];
			g_drag.planeU = frame.axes[(i + 1) % 3];
			g_drag.planeV = frame.axes[(i + 2) % 3];
			if (!RayPlane(frame.rayOrigin, frame.rayDirection, frame.origin, g_drag.planeNormal, g_drag.startHit)) {
				g_drag.active = false;
			}
		} else {
			g_drag.planeNormal = frame.view.cameraForward;
			g_drag.planeU = frame.view.cameraRight;
			g_drag.planeV = frame.view.cameraUp;
			if (!RayPlane(frame.rayOrigin, frame.rayDirection, frame.origin, g_drag.planeNormal, g_drag.startHit)) {
				g_drag.active = false;
			}
		}
		break;

	case Operation::Rotate: {
		g_drag.rotAxis = (handle == kHandleScreen)
			? frame.view.cameraForward
			: frame.axes[handle - kHandleAxis];
		g_drag.rotU = AnyPerpendicular(g_drag.rotAxis);
		g_drag.rotV = Cross(g_drag.rotAxis, g_drag.rotU);

		Vector3 hit{};
		if (!RayPlane(frame.rayOrigin, frame.rayDirection, frame.origin, g_drag.rotAxis, hit)) {
			g_drag.active = false;
			break;
		}
		const Vector3 offset = hit - frame.origin;
		g_drag.lastAngle = std::atan2(Dot(offset, g_drag.rotV), Dot(offset, g_drag.rotU));
		g_drag.accumAngle = 0.0f;
		g_drag.appliedAngle = 0.0f;
		break;
	}

	case Operation::Scale:
		if (handle == kHandleScreen) {
			g_drag.startRadius = Len2(Sub2(frame.mouse, frame.originScreen));
		} else {
			ImVec2 axisEnd{};
			if (!WorldToScreen(frame.view, frame.origin + frame.axes[handle - kHandleAxis] * frame.size, axisEnd)) {
				g_drag.active = false;
				break;
			}
			const ImVec2 toEnd = Sub2(axisEnd, frame.originScreen);
			g_drag.axisScreenLen = (std::max)(Len2(toEnd), 1.0f);
			g_drag.axisScreenDir = Norm2(toEnd);
			g_drag.startProjection = Dot2(Sub2(frame.mouse, frame.originScreen), g_drag.axisScreenDir);
		}
		break;
	}
}

void ApplyTranslateDrag(const Frame& frame, WorldTransform* transform)
{
	const int handle = g_drag.handle;
	Vector3 worldDelta{};

	if (handle >= kHandleAxis && handle < kHandleAxis + 3) {
		const Vector3& axis = g_drag.axes[handle - kHandleAxis];
		float t = 0.0f;
		if (!ClosestPointOnAxis(g_drag.origin, axis, frame.rayOrigin, frame.rayDirection, t)) {
			return;
		}
		float amount = t - g_drag.startAxisT;
		if (SnapActive(Operation::Translate)) {
			amount = SnapTo(amount, g_snapTranslateValue);
		}
		worldDelta = axis * amount;
	} else {
		Vector3 hit{};
		if (!RayPlane(frame.rayOrigin, frame.rayDirection, g_drag.origin, g_drag.planeNormal, hit)) {
			return;
		}
		const Vector3 moved = hit - g_drag.startHit;
		float alongU = Dot(moved, g_drag.planeU);
		float alongV = Dot(moved, g_drag.planeV);
		if (SnapActive(Operation::Translate)) {
			alongU = SnapTo(alongU, g_snapTranslateValue);
			alongV = SnapTo(alongV, g_snapTranslateValue);
		}
		worldDelta = g_drag.planeU * alongU + g_drag.planeV * alongV;
	}

	// ワールドでの移動量を親のローカル空間へ落としてから足す
	const Vector3 localDelta = g_drag.hasParent
		? TransformNormal(worldDelta, g_drag.invParentWorld)
		: worldDelta;
	transform->GetTranslation() = g_drag.startTranslation + localDelta;
}

void ApplyRotateDrag(const Frame& frame, WorldTransform* transform)
{
	Vector3 hit{};
	if (!RayPlane(frame.rayOrigin, frame.rayDirection, g_drag.origin, g_drag.rotAxis, hit)) {
		return;
	}
	const Vector3 offset = hit - g_drag.origin;
	if (Length(offset) < 1e-4f) {
		return;
	}

	// 1周を跨いでも巻き戻らないよう、前フレームからの差分を積算する
	const float angle = std::atan2(Dot(offset, g_drag.rotV), Dot(offset, g_drag.rotU));
	float step = angle - g_drag.lastAngle;
	while (step > kPi) step -= 2.0f * kPi;
	while (step < -kPi) step += 2.0f * kPi;
	g_drag.lastAngle = angle;
	g_drag.accumAngle += step;

	float applied = g_drag.accumAngle;
	if (SnapActive(Operation::Rotate)) {
		applied = SnapTo(applied, g_snapRotateDegree * kPi / 180.0f);
	}
	g_drag.appliedAngle = applied;

	// ワールド軸まわりの回転 Δ を、ローカル回転への掛け算に直す。
	//   q_world = q_parent * q_local なので q_local' = (q_parent^-1 Δ q_parent) * q_local
	// 括弧の中は「Δ の軸を親空間へ移したもの」と等しい
	const Vector3 axisInParent = g_drag.hasParent
		? SafeNormalize(TransformNormal(g_drag.rotAxis, g_drag.invParentWorld), g_drag.rotAxis)
		: g_drag.rotAxis;

	const Quaternion delta = MakeRotateAxisAngleQuaternion(axisInParent, applied);
	transform->GetRotation() = SafeNormalizeQuat(delta * g_drag.startRotation);
}

void ApplyScaleDrag(const Frame& frame, WorldTransform* transform)
{
	Vector3 newScale = g_drag.startScale;

	if (g_drag.handle == kHandleScreen) {
		const float radius = Len2(Sub2(frame.mouse, g_drag.startOriginScreen));
		const float factor = (std::max)(0.001f, 1.0f + (radius - g_drag.startRadius) / kGizmoPixels);
		newScale = g_drag.startScale * factor;
		if (SnapActive(Operation::Scale)) {
			newScale.x = SnapTo(newScale.x, g_snapScaleValue);
			newScale.y = SnapTo(newScale.y, g_snapScaleValue);
			newScale.z = SnapTo(newScale.z, g_snapScaleValue);
		}
	} else {
		const int i = g_drag.handle - kHandleAxis;
		const float projection = Dot2(Sub2(frame.mouse, g_drag.startOriginScreen), g_drag.axisScreenDir);
		const float factor = (std::max)(0.001f,
			1.0f + (projection - g_drag.startProjection) / g_drag.axisScreenLen);

		float* component = (i == 0) ? &newScale.x : (i == 1) ? &newScale.y : &newScale.z;
		*component *= factor;
		if (SnapActive(Operation::Scale)) {
			*component = SnapTo(*component, g_snapScaleValue);
		}
	}

	transform->GetScale() = newScale;
}

// --- 読み取り値の表示 ---

void DrawReadout(const Frame& frame, WorldTransform* transform)
{
	char text[128];
	switch (g_drag.operation) {
	case Operation::Translate: {
		const Vector3& t = transform->GetTranslation();
		snprintf(text, sizeof(text), "位置  %.2f, %.2f, %.2f", t.x, t.y, t.z);
		break;
	}
	case Operation::Rotate: {
		const char* axisName = (g_drag.handle == kHandleScreen) ? "画面" : kAxisName[g_drag.handle - kHandleAxis];
		snprintf(text, sizeof(text), "%s 軸  %+.1f°", axisName, g_drag.appliedAngle * 180.0f / kPi);
		break;
	}
	case Operation::Scale: {
		const Vector3& s = transform->GetScale();
		snprintf(text, sizeof(text), "拡縮  %.3f, %.3f, %.3f", s.x, s.y, s.z);
		break;
	}
	}

	const ImVec2 textSize = ImGui::CalcTextSize(text);
	const ImVec2 position(frame.originScreen.x + 18.0f, frame.originScreen.y - textSize.y - 14.0f);
	frame.drawList->AddRectFilled(
		ImVec2(position.x - 5.0f, position.y - 3.0f),
		ImVec2(position.x + textSize.x + 5.0f, position.y + textSize.y + 3.0f), kReadoutBg, 3.0f);
	frame.drawList->AddText(position, kReadoutText, text);
}

// --- ゲームビュー内のツールバー ---

void DrawToolbar(const ImVec2& imagePos)
{
	const ImVec2 restoreCursor = ImGui::GetCursorScreenPos();
	ImGui::SetCursorScreenPos(ImVec2(imagePos.x + 8.0f, imagePos.y + 8.0f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.17f, 0.88f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.36f, 0.95f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.42f, 0.50f, 1.00f));

	const auto modeButton = [](const char* label, Operation operation, const char* tooltip) {
		const bool selected = (g_operation == operation);
		if (selected) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.80f, 0.95f));
		}
		if (ImGui::Button(label)) {
			g_operation = operation;
		}
		if (selected) {
			ImGui::PopStyleColor();
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", tooltip);
		}
		ImGui::SameLine(0.0f, 4.0f);
	};

	modeButton(" 移動 ", Operation::Translate, "移動 (Ctrl+1)");
	modeButton(" 回転 ", Operation::Rotate, "回転 (Ctrl+2)");
	modeButton(" 拡縮 ", Operation::Scale, "拡縮 (Ctrl+3)  ※常にローカル軸");

	ImGui::Dummy(ImVec2(6.0f, 0.0f));
	ImGui::SameLine(0.0f, 4.0f);

	// 拡縮は常にローカル軸なので、そのときだけ切り替えを伏せる
	ImGui::BeginDisabled(g_operation == Operation::Scale);
	// ラベルは切り替わるが ID は固定にしておく（##以降は表示されずIDにだけ効く）
	if (ImGui::Button((g_operation == Operation::Scale || g_space == Space::Local)
		? " ローカル ##space" : " ワールド ##space")) {
		g_space = (g_space == Space::World) ? Space::Local : Space::World;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("基準にする座標系 (Ctrl+L)");
	}

	ImGui::SameLine(0.0f, 4.0f);

	bool* snapFlag = (g_operation == Operation::Translate) ? &g_snapTranslate
		: (g_operation == Operation::Rotate) ? &g_snapRotate
		: &g_snapScale;
	const float snapValue = (g_operation == Operation::Translate) ? g_snapTranslateValue
		: (g_operation == Operation::Rotate) ? g_snapRotateDegree
		: g_snapScaleValue;

	// Push/Pop の判定はボタンを出す前に確定させる。
	// ボタン自身が *snapFlag を反転させるので、後ろで読むと Push と Pop の数がずれる
	const bool snapOn = *snapFlag;
	if (snapOn) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.80f, 0.95f));
	}
	char snapLabel[64];
	snprintf(snapLabel, sizeof(snapLabel), " スナップ %.2f ##snapToggle", snapValue);
	if (ImGui::Button(snapLabel)) {
		*snapFlag = !*snapFlag;
	}
	if (snapOn) {
		ImGui::PopStyleColor();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("刻み幅は Gizmo メニューで変えられる");
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();

	// SetCursorScreenPos で動かしたままアイテムを1つも出さずに End() すると
	// ImGui が「境界を広げる意図か」と assert するので、空のアイテムで締める
	ImGui::SetCursorScreenPos(restoreCursor);
	ImGui::Dummy(ImVec2(0.0f, 0.0f));
}

void CancelDrag()
{
	g_drag.active = false;
	g_drag.handle = kHandleNone;
}

} // namespace

// --- 状態 ---

bool EditorGizmo::IsEnabled() { return g_enabled; }
void EditorGizmo::SetEnabled(bool enabled)
{
	g_enabled = enabled;
	if (!enabled) {
		CancelDrag();
	}
}

EditorGizmo::Operation EditorGizmo::GetOperation() { return g_operation; }
void EditorGizmo::SetOperation(Operation operation) { g_operation = operation; }

EditorGizmo::Space EditorGizmo::GetSpace() { return g_space; }
void EditorGizmo::SetSpace(Space space) { g_space = space; }

bool EditorGizmo::IsUsing() { return g_drag.active; }
bool EditorGizmo::IsOver() { return g_isOver || g_drag.active; }

// --- 本体 ---

void EditorGizmo::DrawOverlay(const ImVec2& imagePos, const ImVec2& imageSize)
{
	g_isOver = false;

	if (!g_enabled || imageSize.x <= 1.0f || imageSize.y <= 1.0f) {
		CancelDrag();
		return;
	}

	if (g_showToolbar) {
		DrawToolbar(imagePos);
	}

	Object3d* object = Editor::GetSelectedObject();
	WorldTransform* transform = object ? object->GetWorldTransform() : nullptr;
	if (!transform) {
		CancelDrag();
		return;
	}

	CameraManager* cameraManager = Editor::Ctx().cameraManager;
	BaseCamera* camera = cameraManager ? cameraManager->GetActiveCamera() : nullptr;
	if (!camera) {
		CancelDrag();
		return;
	}

	Frame frame;
	frame.view.viewProjection = camera->GetViewProjectionMatrix();
	frame.view.inverseViewProjection = Inverse(frame.view.viewProjection);
	// Inverse() は行列式のゼロ割りを見ていない。潰れた行列だと NaN が伝播するので弾く
	if (!std::isfinite(frame.view.inverseViewProjection.m[0][0])) {
		CancelDrag();
		return;
	}

	// カメラのワールド行列は行ベクトル。0行目が右、1行目が上、2行目が前
	const Matrix4x4& cameraWorld = camera->GetWorldMatrix();
	frame.view.cameraRight = SafeNormalize({ cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2] }, kWorldAxis[0]);
	frame.view.cameraUp = SafeNormalize({ cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2] }, kWorldAxis[1]);
	frame.view.cameraForward = SafeNormalize({ cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2] }, kWorldAxis[2]);
	frame.view.cameraPosition = { cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2] };
	frame.view.imagePos = imagePos;
	frame.view.imageSize = imageSize;

	frame.mouse = ImGui::GetIO().MousePos;
	ScreenToRay(frame.view, frame.mouse, frame.rayOrigin, frame.rayDirection);

	// 親の情報。ドラッグ中は開始時に固定したものを使うので、ここでは開始判定用
	const bool hasParent = (transform->GetParent() != nullptr);
	const Matrix4x4 parentWorld = hasParent ? transform->GetParent()->GetMatWorld() : MakeIdentity4x4();

	// ドラッグの適用はギズモの形を決めるより先。
	// そうしないと、動かした結果が1フレーム遅れて描かれる
	if (g_drag.active) {
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			CancelDrag();
		} else {
			switch (g_drag.operation) {
			case Operation::Translate: ApplyTranslateDrag(frame, transform); break;
			case Operation::Rotate:    ApplyRotateDrag(frame, transform); break;
			case Operation::Scale:     ApplyScaleDrag(frame, transform); break;
			}
		}
	}

	// ギズモの位置と軸。matWorld_ はゲーム側の更新でしか動かないので、
	// 表示に使う行列はローカル値から自前で組み直す（ポーズ中でも即座に追従する）
	const Matrix4x4 localMatrix = MakeAffineMatrix(
		transform->GetScale(), SafeNormalizeQuat(transform->GetRotation()), transform->GetTranslation());
	const Matrix4x4 worldMatrix = hasParent ? (localMatrix * parentWorld) : localMatrix;

	frame.origin = { worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2] };

	if (g_drag.active) {
		// ドラッグ中に軸が動くと操作が暴れるので、開始時の向きを使い続ける
		for (int i = 0; i < 3; ++i) {
			frame.axes[i] = g_drag.axes[i];
		}
	} else {
		// 拡縮はローカル軸にしか効かないので、World 指定でもローカル軸を出す
		const bool useLocalAxes = (g_operation == Operation::Scale) || (g_space == Space::Local);
		for (int i = 0; i < 3; ++i) {
			frame.axes[i] = useLocalAxes
				? SafeNormalize({ worldMatrix.m[i][0], worldMatrix.m[i][1], worldMatrix.m[i][2] }, kWorldAxis[i])
				: kWorldAxis[i];
		}
	}

	if (!WorldToScreen(frame.view, frame.origin, frame.originScreen)) {
		CancelDrag();
		return;
	}
	frame.size = ComputeGizmoWorldSize(frame.view, frame.origin);
	if (frame.size <= 0.0f) {
		CancelDrag();
		return;
	}

	// ツールバーのボタンに乗っているときはギズモを掴ませない
	const bool insideImage = frame.mouse.x >= imagePos.x && frame.mouse.x <= imagePos.x + imageSize.x
		&& frame.mouse.y >= imagePos.y && frame.mouse.y <= imagePos.y + imageSize.y;
	frame.canInteract = !g_drag.active && insideImage
		&& ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
		&& !ImGui::IsAnyItemHovered();

	frame.drawList = ImGui::GetWindowDrawList();
	frame.drawList->PushClipRect(imagePos, ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y), true);

	const int activeHandle = g_drag.active ? g_drag.handle : kHandleNone;
	const Operation drawnOperation = g_drag.active ? g_drag.operation : g_operation;

	int hovered = kHandleNone;
	switch (drawnOperation) {
	case Operation::Translate: hovered = PickAndDrawTranslate(frame, activeHandle); break;
	case Operation::Rotate:    hovered = PickAndDrawRotate(frame, activeHandle); break;
	case Operation::Scale:     hovered = PickAndDrawScale(frame, activeHandle); break;
	}
	g_isOver = (hovered != kHandleNone);

	if (g_drag.active) {
		DrawReadout(frame, transform);
	}

	frame.drawList->PopClipRect();

	if (frame.canInteract && hovered != kHandleNone && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		BeginDrag(frame, transform, hovered, hasParent, parentWorld);
	}
}

// --- メニュー ---

void EditorGizmo::DrawMenu()
{
	ImGui::MenuItem("ギズモを表示", "Ctrl+G", &g_enabled);
	ImGui::MenuItem("ツールバーを重ねる", nullptr, &g_showToolbar);

	ImGui::Separator();

	int operation = static_cast<int>(g_operation);
	if (ImGui::RadioButton("移動##op", &operation, static_cast<int>(Operation::Translate))) {
		g_operation = Operation::Translate;
	}
	if (ImGui::RadioButton("回転##op", &operation, static_cast<int>(Operation::Rotate))) {
		g_operation = Operation::Rotate;
	}
	if (ImGui::RadioButton("拡縮##op", &operation, static_cast<int>(Operation::Scale))) {
		g_operation = Operation::Scale;
	}

	ImGui::Separator();

	int space = static_cast<int>(g_space);
	ImGui::BeginDisabled(g_operation == Operation::Scale);
	if (ImGui::RadioButton("ワールド軸##space", &space, static_cast<int>(Space::World))) {
		g_space = Space::World;
	}
	if (ImGui::RadioButton("ローカル軸##space", &space, static_cast<int>(Space::Local))) {
		g_space = Space::Local;
	}
	ImGui::EndDisabled();
	if (g_operation == Operation::Scale) {
		ImGui::TextDisabled("※ 拡縮は常にローカル軸");
	}

	ImGui::Separator();

	if (ImGui::BeginMenu("スナップ")) {
		ImGui::Checkbox("移動##snap", &g_snapTranslate);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(110.0f);
		ImGui::DragFloat("##snapT", &g_snapTranslateValue, 0.05f, 0.01f, 100.0f, "%.2f m");

		ImGui::Checkbox("回転##snap", &g_snapRotate);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(110.0f);
		ImGui::DragFloat("##snapR", &g_snapRotateDegree, 0.5f, 1.0f, 180.0f, "%.0f°");

		ImGui::Checkbox("拡縮##snap", &g_snapScale);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(110.0f);
		ImGui::DragFloat("##snapS", &g_snapScaleValue, 0.01f, 0.01f, 10.0f, "%.2f");

		ImGui::EndMenu();
	}

	ImGui::Separator();
	ImGui::MenuItem("W / E / R でも切り替える", nullptr, &g_useUnityKeys);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Unity 風のキー割り当て。\nゲームの移動入力と重なるので既定はオフ");
	}

	ImGui::Separator();
	ImGui::TextDisabled("Ctrl+1 / 2 / 3   移動 / 回転 / 拡縮");
	ImGui::TextDisabled("Ctrl+L           ワールド ⇔ ローカル");
	ImGui::TextDisabled("Ctrl+G           ギズモの表示切替");
	ImGui::TextDisabled("ドラッグ中 Ctrl  スナップを一時的に反転");
}

void EditorGizmo::HandleShortcuts()
{
	// ドラッグ中の Ctrl はスナップの一時反転に使う。
	// 押しっぱなしを見たいので Shortcut ではなく生のキー状態
	g_snapOverride = g_drag.active && ImGui::GetIO().KeyCtrl;

	// ドラッグ中は Ctrl がスナップ用なので、モード切替は受け付けない
	if (!g_drag.active) {
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_1, ImGuiInputFlags_RouteGlobal)) {
			g_operation = Operation::Translate;
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_2, ImGuiInputFlags_RouteGlobal)) {
			g_operation = Operation::Rotate;
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_3, ImGuiInputFlags_RouteGlobal)) {
			g_operation = Operation::Scale;
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_L, ImGuiInputFlags_RouteGlobal)) {
			g_space = (g_space == Space::World) ? Space::Local : Space::World;
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_G, ImGuiInputFlags_RouteGlobal)) {
			SetEnabled(!g_enabled);
		}
	}

	// W/E/R は既定でオフ。ゲームの移動入力と衝突するため
	if (g_useUnityKeys && !ImGui::GetIO().WantTextInput) {
		if (ImGui::Shortcut(ImGuiKey_W, ImGuiInputFlags_RouteGlobal)) {
			g_operation = Operation::Translate;
		}
		if (ImGui::Shortcut(ImGuiKey_E, ImGuiInputFlags_RouteGlobal)) {
			g_operation = Operation::Rotate;
		}
		if (ImGui::Shortcut(ImGuiKey_R, ImGuiInputFlags_RouteGlobal)) {
			g_operation = Operation::Scale;
		}
	}
}

// --- 保存と読み込み ---

void EditorGizmo::LoadSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);
	gv.LoadFile(kDirectoryName, kGroupName);

	const auto loadBool = [&gv](const char* key, bool& target) {
		if (gv.HasItem(kGroupName, key)) {
			target = gv.GetValueRef<bool>(kGroupName, key);
		}
	};
	const auto loadFloat = [&gv](const char* key, float& target) {
		if (gv.HasItem(kGroupName, key)) {
			target = gv.GetValueRef<float>(kGroupName, key);
		}
	};

	loadBool("Enabled", g_enabled);
	loadBool("ShowToolbar", g_showToolbar);
	loadBool("UseUnityKeys", g_useUnityKeys);
	loadBool("SnapTranslate", g_snapTranslate);
	loadBool("SnapRotate", g_snapRotate);
	loadBool("SnapScale", g_snapScale);
	loadFloat("SnapTranslateValue", g_snapTranslateValue);
	loadFloat("SnapRotateDegree", g_snapRotateDegree);
	loadFloat("SnapScaleValue", g_snapScaleValue);

	if (gv.HasItem(kGroupName, "Operation")) {
		const int32_t value = gv.GetValueRef<int32_t>(kGroupName, "Operation");
		if (value >= 0 && value <= static_cast<int32_t>(Operation::Scale)) {
			g_operation = static_cast<Operation>(value);
		}
	}
	if (gv.HasItem(kGroupName, "Space")) {
		const int32_t value = gv.GetValueRef<int32_t>(kGroupName, "Space");
		if (value >= 0 && value <= static_cast<int32_t>(Space::Local)) {
			g_space = static_cast<Space>(value);
		}
	}
}

void EditorGizmo::SaveSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);

	gv.SetValue(kGroupName, "Enabled", g_enabled);
	gv.SetValue(kGroupName, "ShowToolbar", g_showToolbar);
	gv.SetValue(kGroupName, "UseUnityKeys", g_useUnityKeys);
	gv.SetValue(kGroupName, "SnapTranslate", g_snapTranslate);
	gv.SetValue(kGroupName, "SnapRotate", g_snapRotate);
	gv.SetValue(kGroupName, "SnapScale", g_snapScale);
	gv.SetValue(kGroupName, "SnapTranslateValue", g_snapTranslateValue);
	gv.SetValue(kGroupName, "SnapRotateDegree", g_snapRotateDegree);
	gv.SetValue(kGroupName, "SnapScaleValue", g_snapScaleValue);
	gv.SetValue(kGroupName, "Operation", static_cast<int32_t>(g_operation));
	gv.SetValue(kGroupName, "Space", static_cast<int32_t>(g_space));

	gv.SaveFile(kDirectoryName, kGroupName);
}

#endif // _DEBUG
