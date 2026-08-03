#include "EditorCamera.h"
#ifdef _DEBUG

#include "EditorContext.h"
#include "EditorMenuBar.h"

#include "Debugger/GlobalVariables.h"
#include "Input/Input.h"
#include "Math/MathUtils.h"
#include "Utility/DeltaTime.h"
#include "World3D/Camera/BaseCamera.h"
#include "World3D/Camera/CameraManager.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>

namespace {

constexpr const char* kDirectoryName = "Editor";
constexpr const char* kGroupName = "EditorCamera";
constexpr const char* kCameraName = "EditorDebugCamera";

constexpr float kPi = std::numbers::pi_v<float>;
// 真上・真下で軸が縮退するので、少し手前で止める
constexpr float kMaxPitch = kPi * 0.5f - 0.01f;

constexpr float kMinSpeed = 0.5f;
constexpr float kMaxSpeed = 500.0f;
// ホイール1段あたりの倍率
constexpr float kSpeedStep = 1.15f;

// --- 設定（EditorCamera.json に保存される） ---

float g_speed = 12.0f;          // m/秒
float g_boostScale = 4.0f;      // Ctrl を押している間の倍率
float g_lookSensitivity = 0.0025f; // マウス1カウントあたりのラジアン

// --- 実行時の状態 ---

std::unique_ptr<BaseCamera> g_camera;
bool g_active = false;
bool g_flying = false;

/// <summary>ゲームへの入力を止める／戻す。二重に呼んでも無害</summary>
void SetGameInputSuppressed(bool suppressed)
{
	Input::GetInstance().SetSuppressedForEditor(suppressed);
}

/// <summary>今の見た目のカメラから位置・向き・画角を引き継ぐ</summary>
void CopyFromCurrentCamera(BaseCamera& target)
{
	CameraManager* manager = Editor::Ctx().cameraManager;
	if (!manager) {
		return;
	}
	// SetDebugCamera() より前に呼ぶこと。後だと自分自身が返ってくる
	BaseCamera* source = manager->GetActiveCamera();
	if (!source || source == &target) {
		return;
	}

	target.GetTranslate() = source->GetTranslate();
	target.GetRotate() = source->GetRotate();
	target.SetFovY(source->GetFovY());
	target.SetAspectRate(source->GetAspectRate());
	target.SetNearClip(source->GetNearClip());
	target.SetFarClip(source->GetFarClip());
	target.Update();
}

} // namespace

// --- 状態 ---

bool EditorCamera::IsActive() { return g_active; }
bool EditorCamera::IsFlying() { return g_flying; }

void EditorCamera::SetActive(bool active)
{
	if (active == g_active) {
		return;
	}

	CameraManager* manager = Editor::Ctx().cameraManager;
	if (!manager) {
		return;
	}

	if (active) {
		if (!g_camera) {
			g_camera = std::make_unique<BaseCamera>(kCameraName);
		}
		// 割り込む前に今の画をコピーする。順番を逆にすると自分から自分へコピーしてしまう
		CopyFromCurrentCamera(*g_camera);
		manager->SetDebugCamera(g_camera.get());
		g_active = true;
		EditorMenuBar::ShowToast("デバッグカメラ ON（右ドラッグ + WASD / Space / Shift）");
	} else {
		manager->SetDebugCamera(nullptr);
		g_active = false;
		g_flying = false;
		SetGameInputSuppressed(false);
		EditorMenuBar::ShowToast("デバッグカメラ OFF");
	}
}

void EditorCamera::Toggle()
{
	SetActive(!g_active);
}

// --- 駆動 ---

void EditorCamera::Update(bool gameViewHovered)
{
	if (!g_active || !g_camera) {
		if (g_flying) {
			g_flying = false;
			SetGameInputSuppressed(false);
		}
		return;
	}

	Input& input = Input::GetInstance();

	// 掴み始めはゲームビューの上だけ。一度掴んだら離すまで、はみ出しても飛び続ける
	if (g_flying) {
		g_flying = input.IsPressMouseRaw(1);
	} else {
		g_flying = gameViewHovered && input.IsPressMouseRaw(1);
	}
	// 飛んでいる間だけゲームへの入力を止める。
	// 止めっぱなしにすると、カメラを出したままゲームを操作できなくなる
	SetGameInputSuppressed(g_flying);

	if (!g_flying) {
		return;
	}

	// ポーズ中でもカメラは動かしたいので、スケールのかかっていない実時間を使う
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();

	// ── 視点 ──
	const Input::MouseMove mouseMove = input.GetMouseMoveRaw();
	Vector3& rotate = g_camera->GetRotate();
	rotate.y += static_cast<float>(mouseMove.lX) * g_lookSensitivity;
	rotate.x += static_cast<float>(mouseMove.lY) * g_lookSensitivity;
	rotate.x = std::clamp(rotate.x, -kMaxPitch, kMaxPitch);
	rotate.z = 0.0f;

	// ── ホイールで移動速度 ──
	const int32_t wheel = input.GetWheelRaw();
	if (wheel > 0) {
		g_speed = (std::min)(g_speed * kSpeedStep, kMaxSpeed);
	} else if (wheel < 0) {
		g_speed = (std::max)(g_speed / kSpeedStep, kMinSpeed);
	}

	// ── 移動 ──
	// カメラのワールド行列は MakeAffineMatrix(scale, rotate, translate) ＝ Rx * Ry。
	// その2行目（前）と0行目（右）を直に書き下す。
	// BaseCamera::GetRight() は Ry * Rx で組んでいて、この行列とは一致しない
	const float pitch = rotate.x;
	const float yaw = rotate.y;
	const Vector3 forward{ std::cos(pitch) * std::sin(yaw), -std::sin(pitch), std::cos(pitch) * std::cos(yaw) };
	const Vector3 right{ std::cos(yaw), 0.0f, -std::sin(yaw) };
	const Vector3 worldUp{ 0.0f, 1.0f, 0.0f };

	Vector3 direction{};
	if (input.PushKeyRaw(DIK_W)) direction += forward;
	if (input.PushKeyRaw(DIK_S)) direction -= forward;
	if (input.PushKeyRaw(DIK_D)) direction += right;
	if (input.PushKeyRaw(DIK_A)) direction -= right;
	if (input.PushKeyRaw(DIK_SPACE)) direction += worldUp;
	if (input.PushKeyRaw(DIK_LSHIFT) || input.PushKeyRaw(DIK_RSHIFT)) direction -= worldUp;

	if (Length(direction) > 1e-4f) {
		float speed = g_speed;
		if (input.PushKeyRaw(DIK_LCONTROL) || input.PushKeyRaw(DIK_RCONTROL)) {
			speed *= g_boostScale;
		}
		g_camera->GetTranslate() += Normalize(direction) * speed * deltaTime;
	}
}

void EditorCamera::Finalize()
{
	if (CameraManager* manager = Editor::Ctx().cameraManager) {
		manager->SetDebugCamera(nullptr);
	}
	SetGameInputSuppressed(false);
	g_active = false;
	g_flying = false;
	g_camera.reset();
}

// --- 表示 ---

void EditorCamera::DrawBadge(const ImVec2& imagePos, const ImVec2& imageSize)
{
	if (!g_active) {
		return;
	}

	char text[128];
	if (g_flying) {
		snprintf(text, sizeof(text), "デバッグカメラ  飛行中  %.1f m/s", g_speed);
	} else {
		snprintf(text, sizeof(text), "デバッグカメラ  右ドラッグで飛行  (F9で戻す)");
	}

	const ImVec2 textSize = ImGui::CalcTextSize(text);
	const ImVec2 position(imagePos.x + imageSize.x - textSize.x - 16.0f, imagePos.y + 8.0f);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(
		ImVec2(position.x - 8.0f, position.y - 4.0f),
		ImVec2(position.x + textSize.x + 8.0f, position.y + textSize.y + 4.0f),
		g_flying ? IM_COL32(180, 90, 20, 220) : IM_COL32(24, 24, 30, 200), 4.0f);
	drawList->AddText(position, IM_COL32(245, 245, 250, 255), text);
}

void EditorCamera::DrawSettings()
{
	bool active = g_active;
	if (ImGui::Checkbox("デバッグカメラ (F9)", &active)) {
		SetActive(active);
	}

	if (g_active) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), g_flying ? "飛行中" : "待機");
	}

	ImGui::TextDisabled("右ドラッグ中だけ操作を受ける。その間ゲームへの入力は止まる");
	ImGui::TextDisabled("WASD 前後左右 / Space 上 / Shift 下 / Ctrl ダッシュ / ホイール 速度");

	ImGui::SetNextItemWidth(150.0f);
	ImGui::DragFloat("移動速度 (m/s)", &g_speed, 0.2f, kMinSpeed, kMaxSpeed, "%.1f");
	ImGui::SetNextItemWidth(150.0f);
	ImGui::DragFloat("ダッシュ倍率", &g_boostScale, 0.1f, 1.0f, 20.0f, "x%.1f");
	ImGui::SetNextItemWidth(150.0f);
	ImGui::DragFloat("視点感度", &g_lookSensitivity, 0.0001f, 0.0002f, 0.02f, "%.4f");

	ImGui::BeginDisabled(!g_active || !g_camera);
	if (ImGui::Button("今のゲームカメラの位置へ")) {
		// 一度外してから引き直さないと、コピー元が自分自身になる
		if (CameraManager* manager = Editor::Ctx().cameraManager) {
			manager->SetDebugCamera(nullptr);
			CopyFromCurrentCamera(*g_camera);
			manager->SetDebugCamera(g_camera.get());
		}
	}
	ImGui::EndDisabled();

	if (g_camera) {
		const Vector3& position = g_camera->GetTranslate();
		const Vector3& rotate = g_camera->GetRotate();
		ImGui::TextDisabled("位置 %.2f, %.2f, %.2f", position.x, position.y, position.z);
		ImGui::TextDisabled("向き pitch %.1f°  yaw %.1f°",
			rotate.x * 180.0f / kPi, rotate.y * 180.0f / kPi);
	}
}

// --- 保存と読み込み ---

void EditorCamera::LoadSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);
	gv.LoadFile(kDirectoryName, kGroupName);

	if (gv.HasItem(kGroupName, "Speed")) {
		g_speed = std::clamp(gv.GetValueRef<float>(kGroupName, "Speed"), kMinSpeed, kMaxSpeed);
	}
	if (gv.HasItem(kGroupName, "BoostScale")) {
		g_boostScale = gv.GetValueRef<float>(kGroupName, "BoostScale");
	}
	if (gv.HasItem(kGroupName, "LookSensitivity")) {
		g_lookSensitivity = gv.GetValueRef<float>(kGroupName, "LookSensitivity");
	}
}

void EditorCamera::SaveSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);
	gv.SetValue(kGroupName, "Speed", g_speed);
	gv.SetValue(kGroupName, "BoostScale", g_boostScale);
	gv.SetValue(kGroupName, "LookSensitivity", g_lookSensitivity);
	gv.SaveFile(kDirectoryName, kGroupName);
}

#endif // _DEBUG
