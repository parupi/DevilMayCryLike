#include "EditorPicking.h"
#ifdef _DEBUG

#include "EditorContext.h"
#include "EditorGizmo.h"
#include "EditorSelection.h"
#include "EditorViewMath.h"

#include "Debugger/GlobalVariables.h"
#include "Math/MathUtils.h"
#include "World3D/Object/Model/Model.h"
#include "World3D/Object/Model/SkinnedModel.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "World3D/WorldTransform.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace {

constexpr const char* kDirectoryName = "Editor";
constexpr const char* kGroupName = "EditorPicking";

// 三角形がこれを超えるモデルは、焼かずにAABBだけで判定する。
// 1つで 600万頂点 = 72MB になる手前で諦める、くらいの目安
constexpr size_t kMaxTrianglesPerModel = 200000;

// 同じ場所をもう一度クリックしたと見なす距離（ピクセル）
constexpr float kCycleSlopPixels = 4.0f;

constexpr ImU32 kOutlineColor = IM_COL32(255, 170, 40, 200);

bool g_enabled = true;
bool g_outlineEnabled = true;

// 重なったオブジェクトを手前から順に切り替えるための記憶
ImVec2 g_lastClickPos{ -1000.0f, -1000.0f };
std::string g_lastPickedName;

/// <summary>ピック用に焼いたモデルの形</summary>
struct PickGeometry {
	Vector3 aabbMin{ FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 aabbMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
	// 3頂点ずつの三角形リスト。空ならAABBだけで判定する（スキンモデル・巨大モデル）
	std::vector<Vector3> triangles;
	bool valid = false;
};

std::unordered_map<const BaseModel*, PickGeometry> g_geometry;

void ExpandAABB(PickGeometry& geometry, const Vector3& p)
{
	geometry.aabbMin = Min(geometry.aabbMin, p);
	geometry.aabbMax = Max(geometry.aabbMax, p);
}

/// <summary>頂点配列とインデックス配列から三角形を積む。インデックスが無ければ3つずつ束ねる</summary>
void AppendTriangles(PickGeometry& geometry,
	const std::vector<VertexData>& vertices, const std::vector<int32_t>& indices)
{
	const auto positionAt = [&vertices](size_t index) {
		const Vector4& p = vertices[index].position;
		return Vector3{ p.x, p.y, p.z };
	};

	if (indices.empty()) {
		const size_t count = (vertices.size() / 3) * 3;
		for (size_t i = 0; i < count; ++i) {
			geometry.triangles.push_back(positionAt(i));
		}
		return;
	}

	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		const int32_t i0 = indices[i];
		const int32_t i1 = indices[i + 1];
		const int32_t i2 = indices[i + 2];
		if (i0 < 0 || i1 < 0 || i2 < 0) {
			continue;
		}
		if (static_cast<size_t>(i0) >= vertices.size()
			|| static_cast<size_t>(i1) >= vertices.size()
			|| static_cast<size_t>(i2) >= vertices.size()) {
			continue;
		}
		geometry.triangles.push_back(positionAt(static_cast<size_t>(i0)));
		geometry.triangles.push_back(positionAt(static_cast<size_t>(i1)));
		geometry.triangles.push_back(positionAt(static_cast<size_t>(i2)));
	}
}

/// <summary>
/// モデルのピック用データを引く。初回だけ焼いてキャッシュする。
/// 使えるデータが無いモデルでは nullptr。
/// </summary>
const PickGeometry* GetGeometry(BaseModel* model)
{
	if (!model) {
		return nullptr;
	}

	const auto found = g_geometry.find(model);
	if (found != g_geometry.end()) {
		return found->second.valid ? &found->second : nullptr;
	}

	PickGeometry geometry;

	if (auto* staticModel = dynamic_cast<Model*>(model)) {
		// GetModelData() は値を返すが、モデル1つにつきここで1度きり
		const ModelData data = staticModel->GetModelData();
		for (const MeshData& mesh : data.meshes) {
			for (const VertexData& vertex : mesh.vertices) {
				ExpandAABB(geometry, { vertex.position.x, vertex.position.y, vertex.position.z });
			}
			AppendTriangles(geometry, mesh.vertices, mesh.indices);
		}
		if (geometry.triangles.size() / 3 > kMaxTrianglesPerModel) {
			geometry.triangles.clear();
			geometry.triangles.shrink_to_fit();
		}
	} else if (auto* skinnedModel = dynamic_cast<SkinnedModel*>(model)) {
		// 頂点はバインドポーズのまま。アニメで動くので三角形は当てにならず、AABBだけ使う
		const SkinnedModelData data = skinnedModel->GetModelData();
		for (const SkinnedMeshData& mesh : data.meshes) {
			for (const VertexData& vertex : mesh.vertices) {
				ExpandAABB(geometry, { vertex.position.x, vertex.position.y, vertex.position.z });
			}
		}
	}

	geometry.valid = (geometry.aabbMin.x <= geometry.aabbMax.x);
	const auto inserted = g_geometry.emplace(model, std::move(geometry));
	return inserted.first->second.valid ? &inserted.first->second : nullptr;
}

// --- 交差判定 ---

/// <summary>
/// 点をアフィン行列で変換する。
/// MathUtils の Transform() は w=0 で assert するので、ここでは自前で持つ
/// </summary>
Vector3 TransformPoint(const Vector3& v, const Matrix4x4& m)
{
	return {
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2],
	};
}

/// <summary>
/// レイとAABBのスラブ判定。direction は正規化されていなくてよく、
/// その場合 outDistance は「direction 何本ぶんか」になる。
/// </summary>
bool RayAABB(const Vector3& origin, const Vector3& direction,
	const Vector3& boxMin, const Vector3& boxMax, float& outDistance)
{
	float tMin = 0.0f;
	float tMax = FLT_MAX;

	const float o[3] = { origin.x, origin.y, origin.z };
	const float d[3] = { direction.x, direction.y, direction.z };
	const float lo[3] = { boxMin.x, boxMin.y, boxMin.z };
	const float hi[3] = { boxMax.x, boxMax.y, boxMax.z };

	for (int axis = 0; axis < 3; ++axis) {
		if (std::abs(d[axis]) < 1e-8f) {
			// 軸に平行。スラブの外なら当たらない
			if (o[axis] < lo[axis] || o[axis] > hi[axis]) {
				return false;
			}
			continue;
		}
		const float inv = 1.0f / d[axis];
		float t0 = (lo[axis] - o[axis]) * inv;
		float t1 = (hi[axis] - o[axis]) * inv;
		if (t0 > t1) {
			std::swap(t0, t1);
		}
		tMin = (std::max)(tMin, t0);
		tMax = (std::min)(tMax, t1);
		if (tMin > tMax) {
			return false;
		}
	}

	outDistance = tMin;
	return true;
}

/// <summary>Möller–Trumbore。裏面も拾う（片面ポリゴンのモデルでも選べるように）</summary>
bool RayTriangle(const Vector3& origin, const Vector3& direction,
	const Vector3& v0, const Vector3& v1, const Vector3& v2, float& outDistance)
{
	const Vector3 edge1 = v1 - v0;
	const Vector3 edge2 = v2 - v0;
	const Vector3 pvec = Cross(direction, edge2);
	const float det = Dot(edge1, pvec);
	if (std::abs(det) < 1e-9f) {
		return false;
	}

	const float invDet = 1.0f / det;
	const Vector3 tvec = origin - v0;
	const float u = Dot(tvec, pvec) * invDet;
	if (u < 0.0f || u > 1.0f) {
		return false;
	}

	const Vector3 qvec = Cross(tvec, edge1);
	const float v = Dot(direction, qvec) * invDet;
	if (v < 0.0f || u + v > 1.0f) {
		return false;
	}

	const float t = Dot(edge2, qvec) * invDet;
	if (t <= 0.0f) {
		return false;
	}
	outDistance = t;
	return true;
}

/// <summary>レンダラー1つとの交差。当たらなければ false</summary>
bool IntersectRenderer(BaseRenderer* renderer,
	const Vector3& rayOrigin, const Vector3& rayDirection, float& outDistance)
{
	if (!renderer || !renderer->isAlive) {
		return false;
	}
	WorldTransform* transform = renderer->GetWorldTransform();
	const PickGeometry* geometry = GetGeometry(renderer->GetModel());
	if (!transform || !geometry) {
		return false;
	}

	const Matrix4x4 inverseWorld = Inverse(transform->GetMatWorld());
	// スケール0などで潰れた行列。Inverse() はゼロ割りを見ていない
	if (!std::isfinite(inverseWorld.m[0][0])) {
		return false;
	}

	// レイをモデルのローカル空間へ持っていく。
	// 向きは**正規化しない**。そうすると出てくる t がそのままワールドでの距離になり、
	// スケールの違うオブジェクトどうしでも前後を比べられる
	const Vector3 localOrigin = TransformPoint(rayOrigin, inverseWorld);
	const Vector3 localDirection = TransformNormal(rayDirection, inverseWorld);

	float aabbDistance = 0.0f;
	if (!RayAABB(localOrigin, localDirection, geometry->aabbMin, geometry->aabbMax, aabbDistance)) {
		return false;
	}

	// 三角形を焼いていないモデル（スキン・巨大）はAABBの当たりをそのまま使う
	if (geometry->triangles.empty()) {
		outDistance = aabbDistance;
		return true;
	}

	float nearest = FLT_MAX;
	const size_t count = geometry->triangles.size();
	for (size_t i = 0; i + 2 < count; i += 3) {
		float distance = 0.0f;
		if (RayTriangle(localOrigin, localDirection,
			geometry->triangles[i], geometry->triangles[i + 1], geometry->triangles[i + 2], distance)) {
			nearest = (std::min)(nearest, distance);
		}
	}
	if (nearest == FLT_MAX) {
		return false;
	}
	outDistance = nearest;
	return true;
}

struct Hit {
	Object3d* object = nullptr;
	float distance = 0.0f;
};

/// <summary>レイに当たったオブジェクトを手前から順に集める</summary>
void CollectHits(const Vector3& rayOrigin, const Vector3& rayDirection, std::vector<Hit>& outHits)
{
	outHits.clear();

	Object3dManager* objectManager = Editor::Ctx().object3dManager;
	if (!objectManager) {
		return;
	}

	for (Object3d* object : objectManager->GetAllObject()) {
		// 見えていないものは選べない。画面に出ていないものを掴んでも混乱するだけ
		if (!object || !object->isAlive || !object->GetIsDraw()) {
			continue;
		}

		float nearest = FLT_MAX;
		for (BaseRenderer* renderer : object->GetRenderers()) {
			float distance = 0.0f;
			if (IntersectRenderer(renderer, rayOrigin, rayDirection, distance)) {
				nearest = (std::min)(nearest, distance);
			}
		}
		if (nearest < FLT_MAX) {
			outHits.push_back({ object, nearest });
		}
	}

	std::sort(outHits.begin(), outHits.end(),
		[](const Hit& a, const Hit& b) { return a.distance < b.distance; });
}

// --- 選択中オブジェクトの枠 ---

void DrawOutline(const EditorView::Context& view, Object3d* object, ImDrawList* drawList)
{
	// 立方体の12辺（頂点番号は下の corner の並びに対応）
	static const int kEdges[12][2] = {
		{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 }, // 下面
		{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 }, // 上面
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }, // 柱
	};

	for (BaseRenderer* renderer : object->GetRenderers()) {
		if (!renderer || !renderer->isAlive) {
			continue;
		}
		WorldTransform* transform = renderer->GetWorldTransform();
		const PickGeometry* geometry = GetGeometry(renderer->GetModel());
		if (!transform || !geometry) {
			continue;
		}

		// ローカルAABBの8隅をそのままワールドへ運ぶ。
		// 回転しているオブジェクトでも、ワールドAABBに広げ直さず本体に沿った箱になる
		const Matrix4x4& world = transform->GetMatWorld();
		ImVec2 corner[8];
		bool cornerVisible[8];
		for (int i = 0; i < 8; ++i) {
			const Vector3 local{
				(i & 1) ? geometry->aabbMax.x : geometry->aabbMin.x,
				(i & 4) ? geometry->aabbMax.y : geometry->aabbMin.y,
				(i & 2) ? geometry->aabbMax.z : geometry->aabbMin.z,
			};
			cornerVisible[i] = EditorView::WorldToScreen(view, TransformPoint(local, world), corner[i]);
		}

		for (const auto& edge : kEdges) {
			// カメラの後ろに回った頂点を含む辺は、伸ばすと画面上で暴れるので描かない
			if (cornerVisible[edge[0]] && cornerVisible[edge[1]]) {
				drawList->AddLine(corner[edge[0]], corner[edge[1]], kOutlineColor, 1.5f);
			}
		}
	}
}

} // namespace

// --- 状態 ---

bool EditorPicking::IsEnabled() { return g_enabled; }
void EditorPicking::SetEnabled(bool enabled) { g_enabled = enabled; }

bool EditorPicking::IsOutlineEnabled() { return g_outlineEnabled; }
void EditorPicking::SetOutlineEnabled(bool enabled) { g_outlineEnabled = enabled; }

void EditorPicking::ClearCache()
{
	g_geometry.clear();
	g_lastPickedName.clear();
}

// --- 本体 ---

Object3d* EditorPicking::Raycast(const Vector3& rayOrigin, const Vector3& rayDirection, float* outDistance)
{
	std::vector<Hit> hits;
	CollectHits(rayOrigin, rayDirection, hits);
	if (hits.empty()) {
		return nullptr;
	}
	if (outDistance) {
		*outDistance = hits.front().distance;
	}
	return hits.front().object;
}

void EditorPicking::HandleGameView(const ImVec2& imagePos, const ImVec2& imageSize)
{
	EditorView::Context view;
	if (!EditorView::Build(view, imagePos, imageSize)) {
		return;
	}

	// 先に枠を描く。クリックの成否とは関係なく、今の選択を示しておきたい
	if (g_outlineEnabled) {
		if (Object3d* selected = Editor::GetSelectedObject()) {
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->PushClipRect(imagePos,
				ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y), true);
			DrawOutline(view, selected, drawList);
			drawList->PopClipRect();
		}
	}

	if (!g_enabled) {
		return;
	}

	// ギズモを掴んでいる／掴もうとしているクリックは横取りしない
	if (EditorGizmo::IsOver()) {
		return;
	}
	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		return;
	}
	const ImVec2 mouse = ImGui::GetIO().MousePos;
	if (!EditorView::IsInsideImage(view, mouse)
		|| !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
		|| ImGui::IsAnyItemHovered()) {
		return;
	}

	Vector3 rayOrigin{};
	Vector3 rayDirection{};
	EditorView::ScreenToRay(view, mouse, rayOrigin, rayDirection);

	std::vector<Hit> hits;
	CollectHits(rayOrigin, rayDirection, hits);

	if (hits.empty()) {
		// 何も無いところを叩いたら選択解除
		Editor::ClearObjectSelection();
		g_lastPickedName.clear();
		g_lastClickPos = mouse;
		return;
	}

	// 同じ場所を続けて叩いたら、重なっている奥のオブジェクトへ送る
	size_t index = 0;
	const bool sameSpot = std::abs(mouse.x - g_lastClickPos.x) <= kCycleSlopPixels
		&& std::abs(mouse.y - g_lastClickPos.y) <= kCycleSlopPixels;
	if (sameSpot && !g_lastPickedName.empty()) {
		for (size_t i = 0; i < hits.size(); ++i) {
			if (hits[i].object->name_ == g_lastPickedName) {
				index = (i + 1) % hits.size();
				break;
			}
		}
	}

	Editor::SelectObject(hits[index].object->name_);
	g_lastPickedName = hits[index].object->name_;
	g_lastClickPos = mouse;
}

// --- メニュー ---

void EditorPicking::DrawMenu()
{
	ImGui::MenuItem("クリックで選択", nullptr, &g_enabled);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("ゲームビューの絵を直接クリックして選ぶ。\n"
			"重なっているときは同じ場所を続けて叩くと奥へ送れる。\n"
			"何も無いところをクリックすると選択解除");
	}
	ImGui::MenuItem("選択中を枠で囲う", nullptr, &g_outlineEnabled);

	ImGui::TextDisabled("焼いたモデル: %zu 件", g_geometry.size());
	if (ImGui::MenuItem("判定用データを作り直す")) {
		ClearCache();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("モデルを差し替えたのに判定がずれているときに使う");
	}
}

// --- 保存と読み込み ---

void EditorPicking::LoadSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);
	gv.LoadFile(kDirectoryName, kGroupName);

	if (gv.HasItem(kGroupName, "Enabled")) {
		g_enabled = gv.GetValueRef<bool>(kGroupName, "Enabled");
	}
	if (gv.HasItem(kGroupName, "Outline")) {
		g_outlineEnabled = gv.GetValueRef<bool>(kGroupName, "Outline");
	}
}

void EditorPicking::SaveSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);
	gv.SetValue(kGroupName, "Enabled", g_enabled);
	gv.SetValue(kGroupName, "Outline", g_outlineEnabled);
	gv.SaveFile(kDirectoryName, kGroupName);
}

#endif // _DEBUG
