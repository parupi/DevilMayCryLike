#include "AppEditor.h"
#ifdef _DEBUG

#include "Windows/AppEditorWindows.h"

#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorHost.h>
#include <Editor/Core/EditorMenuBar.h>
#include <Editor/Core/EditorUndo.h>
#include <Scene/SceneManager.h>
#include <Utility/DeltaTime.h>
#include <World3D/Camera/CameraManager.h>
#include <World3D/Object/Object3d.h>
#include <World3D/Object/Object3dManager.h>

#include <imgui.h>

#include "GameObject/Camera/GameCamera.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Event/BaseEvent.h"
#include "GameObject/Light/StagePointLight.h"
#include "GameObject/Prop/Prop.h"
#include "Stage/SceneLoader.h"
#include "Stage/SceneSaver.h"
#include "Stage/StageDocument.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace {

// ゲーム側のウィンドウを含む配置。エンジンはこれらの名前を知らないので、こちらで持つ
void RegisterLayoutPresets()
{
	Editor::AddLayoutPreset({
		"バトル調整",
		{ "Game" },
		{ "Player", "Attack Editor", "Attack Derivative Editor" },
		{ "Enemy", "Stylish", "GameCamera" },
		{ "TimeManager", "DeltaTime", "Debug Log" },
		0.24f, 0.22f, 0.24f,
		});

	Editor::AddLayoutPreset({
		"カメラ調整",
		{ "Game" },
		{},
		{ "GameCamera", "Player" },
		{ "Debug Log", "TimeManager" },
		0.0f, 0.28f, 0.24f,
		});

	Editor::AddLayoutPreset({
		"VFX作業(ゲーム)",
		{ "Game" },
		{ "Particle Groups" },
		{ "Emitters", "VFX", "HitEffect", "HitPostEffect" },
		{ "Particle Node Graph", "Debug Log" },
		0.18f, 0.26f, 0.38f,
		});
}

// ポイントライトのパラメータ。StagePointLight と ライト付き Prop で中身が同じなので参照で受ける。
// 変更があったら true を返す（呼び出し側が実体のライトへ反映する）
bool DrawPointLightParams(Vector3& color, Vector3& offset, float& intensity, float& radius, float& decay)
{
	bool changed = false;
	changed |= ImGui::ColorEdit3("色", &color.x);
	changed |= ImGui::DragFloat3("オフセット", &offset.x, 0.05f);
	changed |= ImGui::DragFloat("明るさ", &intensity, 0.05f, 0.0f, 100.0f);
	changed |= ImGui::DragFloat("届く距離", &radius, 0.1f, 0.0f, 1000.0f);
	changed |= ImGui::DragFloat("減衰", &decay, 0.01f, 0.0f, 10.0f);
	return changed;
}

const char* ToLabel(EventType type)
{
	switch (type) {
	case EventType::EnemySpawn:  return "敵出現";
	case EventType::Clear:       return "クリア";
	case EventType::ForceBattle: return "強制戦闘";
	case EventType::BossSpawn:   return "ボス出現";
	}
	return "?";
}

// シーンにいる敵の名前を集める。イベントの対象はこの中からしか選ばせない
std::vector<std::string> CollectEnemyNames()
{
	std::vector<std::string> names;
	Object3dManager* objects = Editor::Ctx().object3dManager;
	if (!objects) {
		return names;
	}
	for (Object3d* object : objects->GetAllObject()) {
		if (object && object->isAlive && dynamic_cast<Enemy*>(object)) {
			names.push_back(object->name_);
		}
	}
	std::sort(names.begin(), names.end());
	return names;
}

// イベントの対象（出現させる敵・撃破対象）。
// ここで編集するのは「ステージに書かれる値」で、実行中のイベントの配線には効かない。
// 反映するにはシーンを読み直す（Stage メニューの「保存してシーンをリロード」）
void DrawEventTargets(BaseEvent* event)
{
	// ボス出現は1体だけ使う
	const bool singleTarget = (event->GetType() == EventType::BossSpawn);
	std::vector<std::string>& targets = event->GetTargetNamesRef();

	ImGui::TextDisabled("種類: %s", ToLabel(event->GetType()));
	ImGui::TextDisabled(singleTarget ? "出現させるボス" : "対象の敵");

	int removeIndex = -1;
	for (int i = 0; i < static_cast<int>(targets.size()); ++i) {
		ImGui::PushID(i);
		ImGui::BulletText("%s", targets[i].c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("外す")) {
			removeIndex = i;
		}
		ImGui::PopID();
	}
	if (targets.empty()) {
		ImGui::TextDisabled("(対象が設定されていません)");
	}
	if (removeIndex >= 0) {
		targets.erase(targets.begin() + removeIndex);
	}

	// 追加。既に入っている敵と、単体指定で埋まっている場合は出さない
	const std::vector<std::string> enemyNames = CollectEnemyNames();
	ImGui::BeginDisabled(singleTarget && !targets.empty());
	if (ImGui::BeginCombo("対象を追加", "選ぶ...")) {
		for (const std::string& name : enemyNames) {
			if (std::find(targets.begin(), targets.end(), name) != targets.end()) {
				continue;
			}
			if (ImGui::Selectable(name.c_str())) {
				targets.push_back(name);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();

	if (enemyNames.empty()) {
		ImGui::TextDisabled("シーンに敵がいません");
	}
	ImGui::TextDisabled("※ 変更はシーンを読み直してから効きます");
	ImGui::TextDisabled("   （Stage → 保存してシーンをリロード）");
}

// Inspector の末尾に出す、ゲーム側クラス固有の項目。
// エンジンの Inspector は App の型を知らないので、ここで dynamic_cast する
void DrawStageObjectInspector(Object3d* object)
{
	if (auto* event = dynamic_cast<BaseEvent*>(object)) {
		if (ImGui::CollapsingHeader("イベント", ImGuiTreeNodeFlags_DefaultOpen)) {
			DrawEventTargets(event);
		}
		return;
	}

	if (auto* stageLight = dynamic_cast<StagePointLight*>(object)) {
		if (ImGui::CollapsingHeader("ポイントライト", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (DrawPointLightParams(
				stageLight->GetLightColorRef(), stageLight->GetLightOffsetRef(),
				stageLight->GetLightIntensityRef(), stageLight->GetLightRadiusRef(),
				stageLight->GetLightDecayRef())) {
				stageLight->ApplyLightParams();
			}
		}
		return;
	}

	if (auto* prop = dynamic_cast<Prop*>(object)) {
		if (!ImGui::CollapsingHeader("小物のライト", ImGuiTreeNodeFlags_DefaultOpen)) {
			return;
		}
		bool hasLight = prop->HasLight();
		if (ImGui::Checkbox("ライトを付ける", &hasLight)) {
			prop->SetLightEnabled(hasLight);
		}
		if (!hasLight) {
			return;
		}
		if (DrawPointLightParams(
			prop->GetLightColorRef(), prop->GetLightOffsetRef(),
			prop->GetLightIntensityRef(), prop->GetLightRadiusRef(),
			prop->GetLightDecayRef())) {
			prop->ApplyLightParams();
		}
	}
}

// --- 編集中のステージの記憶（Debugのみ） ---
// 切り替えた保存先が次の起動でも残るようにしておく。
// Release では AppEditor 自体が居ないので、常に既定のステージが読まれる
constexpr const char* kStageSettingsDirectory = "Editor";
constexpr const char* kStageSettingsGroup = "EditorStage";

void LoadStageSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kStageSettingsGroup);
	gv.LoadFile(kStageSettingsDirectory, kStageSettingsGroup);

	if (!gv.HasItem(kStageSettingsGroup, "Path")) {
		return;
	}
	// 消されたステージを指したままだと読み込みで落ちるので、実在するときだけ採用する
	const std::string& path = gv.GetValueRef<std::string>(kStageSettingsGroup, "Path");
	std::error_code ec;
	if (!path.empty() && std::filesystem::is_regular_file(path, ec)) {
		StageDocument::SetPath(path);
	}
}

void SaveStageSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kStageSettingsGroup);
	gv.SetValue(kStageSettingsGroup, "Path", StageDocument::GetPath());
	gv.SaveFile(kStageSettingsDirectory, kStageSettingsGroup);
}

// 保存先を切り替える。次の起動でも同じステージを開けるよう設定に残す
void SwitchStage(const std::string& path)
{
	StageDocument::SetPath(path);
	SaveStageSettings();
}

bool SaveCurrentStage()
{
	const std::string& path = StageDocument::GetPath();
	if (SceneSaver::Save(path)) {
		EditorMenuBar::ShowToast("保存しました: %s", path.c_str());
		return true;
	}
	EditorMenuBar::ShowToast("保存に失敗しました: %s", path.c_str());
	return false;
}

// 「名前を付けて保存」。Resource/Stage/<名前>.json に書き出して、以後そちらを編集対象にする
void DrawSaveAsMenu()
{
	static char newName[64] = "NewStage";

	ImGui::SetNextItemWidth(220.0f);
	ImGui::InputText("ステージ名", newName, sizeof(newName));
	ImGui::TextDisabled("%s", StageDocument::MakePath(newName).c_str());

	const bool validName = StageDocument::IsValidName(newName);
	const bool alreadyExists = validName && StageDocument::Exists(newName);

	if (!validName) {
		ImGui::TextDisabled("名前に使えない文字が入っています");
	} else if (alreadyExists) {
		ImGui::TextDisabled("※ 既にあるステージです。上書きされます");
	}

	ImGui::BeginDisabled(!validName);
	if (ImGui::Button(alreadyExists ? "上書きして保存" : "新規作成", ImVec2(140.0f, 0.0f))) {
		const std::string path = StageDocument::MakePath(newName);
		if (SceneSaver::Save(path)) {
			SwitchStage(path);
			EditorMenuBar::ShowToast("%s を作りました。以後こちらを編集します", path.c_str());
			ImGui::CloseCurrentPopup();
		} else {
			EditorMenuBar::ShowToast("保存に失敗しました: %s", path.c_str());
		}
	}
	ImGui::EndDisabled();
}

// 「開く」。Resource/Stage にある .json を一覧から選んで、そのステージを編集対象にする
void DrawOpenMenu(bool canReload)
{
	const std::vector<std::string> names = StageDocument::ListNames();
	if (names.empty()) {
		ImGui::TextDisabled("(%s にステージがありません)", StageDocument::kDirectory);
		return;
	}

	const std::string current = StageDocument::GetName();
	for (const std::string& name : names) {
		if (ImGui::MenuItem(name.c_str(), nullptr, name == current, canReload)) {
			SwitchStage(StageDocument::MakePath(name));
			EditorUndo::Clear();
			SceneManager::GetInstance().ReloadCurrentScene();
			DeltaTime::SetPaused(true);
			EditorMenuBar::ShowToast("%s を開きました", name.c_str());
		}
	}
	ImGui::Separator();
	ImGui::TextDisabled("開くと読み直すので、保存していない変更は消えます");
}

// ステージデータのメニュー。
// SceneSaver は App の型を dynamic_cast で見るので、エンジンの File メニューではなくこちらに置く
//
// 「編集モード」は独立した状態を持たず、**一時停止しているかどうか**で判断する。
// 動いている間はプレイヤーも敵も位置が変わるので、その状態の保存は許さない。
void DrawStageMenu()
{
	SceneManager& sceneManager = SceneManager::GetInstance();

	const bool isPaused = DeltaTime::IsPaused();
	const bool canReload = !sceneManager.IsSceneChangePending() && !sceneManager.GetCurrentSceneName().empty();

	ImGui::TextDisabled("編集中: %s", StageDocument::GetPath().c_str());
	ImGui::TextDisabled("状態: %s", isPaused ? "編集モード（一時停止中）" : "再生中");
	ImGui::Separator();

	// --- モードの切り替え ---
	// どちらもファイルを経由するので、編集内容が再生で消えることも、
	// 遊んだ結果がステージに焼き付くこともない
	if (ImGui::MenuItem("編集モードに入る", nullptr, false, canReload && !isPaused)) {
		EditorUndo::Clear();
		sceneManager.ReloadCurrentScene();
		DeltaTime::SetPaused(true);
		EditorMenuBar::ShowToast("ステージを読み直して編集モードに入りました");
	}
	if (ImGui::MenuItem("保存して再生", nullptr, false, canReload && isPaused)) {
		if (SaveCurrentStage()) {
			EditorUndo::Clear();
			sceneManager.ReloadCurrentScene();
			DeltaTime::SetPaused(false);
		}
	}

	ImGui::Separator();

	// --- 保存 ---
	ImGui::BeginDisabled(!isPaused);
	if (ImGui::MenuItem("上書き保存")) {
		SaveCurrentStage();
	}
	if (ImGui::BeginMenu("名前を付けて保存")) {
		DrawSaveAsMenu();
		ImGui::EndMenu();
	}
	if (ImGui::MenuItem("保存してシーンをリロード", nullptr, false, canReload)) {
		if (SaveCurrentStage()) {
			EditorUndo::Clear();
			sceneManager.ReloadCurrentScene();
		}
	}
	ImGui::EndDisabled();

	ImGui::Separator();

	if (ImGui::BeginMenu("開く")) {
		DrawOpenMenu(canReload);
		ImGui::EndMenu();
	}

	ImGui::Separator();
	if (!isPaused) {
		ImGui::TextDisabled("再生中は保存できません。敵やプレイヤーが動いた");
		ImGui::TextDisabled("後の位置で焼き付いてしまうためです");
		ImGui::TextDisabled("（F5 で一時停止するか「編集モードに入る」）");
	} else {
		ImGui::TextDisabled("保存されるのは Hierarchy で [保存対象外] が");
		ImGui::TextDisabled("付いていないオブジェクトだけです");
	}
}

} // namespace

Player* AppEditor::FindPlayer()
{
	Object3dManager* objects = Editor::Ctx().object3dManager;
	if (!objects) {
		return nullptr;
	}
	// FindObject は名前が要るうえ、見つからないとログを吐く。
	// 型で探せば名前が変わっても追従できる
	for (Object3d* object : objects->GetAllObject()) {
		if (auto* player = dynamic_cast<Player*>(object)) {
			return player;
		}
	}
	return nullptr;
}

GameCamera* AppEditor::FindGameCamera()
{
	CameraManager* cameras = Editor::Ctx().cameraManager;
	if (!cameras) {
		return nullptr;
	}
	// FindCamera は operator[] で引くので、無い名前を渡すと空要素が生える。
	// 登録済みの名前だけを回すこと
	for (const std::string& name : cameras->GetCameraNames()) {
		if (auto* camera = dynamic_cast<GameCamera*>(cameras->FindCamera(name))) {
			return camera;
		}
	}
	return nullptr;
}

void AppEditor::Register()
{
	// 前回どのステージを編集していたかを復元する（GameScene が読む前に呼ばれる）
	LoadStageSettings();

	RegisterLayoutPresets();

	// ゲーム固有のウィンドウ。エンジン標準のウィンドウの後ろに並ぶ
	Editor::AddWindowDrawer([] { DrawPlayerWindow(); });
	Editor::AddWindowDrawer([] { DrawAttackEditorWindows(); });
	Editor::AddWindowDrawer([] { DrawEnemyWindow(); });
	Editor::AddWindowDrawer([] { DrawCameraWorkWindow(); });
	Editor::AddWindowDrawer([] { DrawStylishWindow(); });
	Editor::AddWindowDrawer([] { DrawHitEffectWindows(); });
	Editor::AddWindowDrawer([] { DrawTrainingWindow(); });

	// ゲーム固有のメニュー。Help の右、再生コントロールの左に並ぶ
	Editor::AddMenu("Stage", [] { DrawStageMenu(); });

	// Inspector の末尾に出すゲーム側クラスの項目（ポイントライトなど）
	Editor::AddInspectorSection([](Object3d* object) { DrawStageObjectInspector(object); });

	// 複製したときに、エンジンが知らないクラス固有の設定を引き継ぐ
	Editor::AddDuplicateHandler([](Object3d* source, Object3d* created) {
		if (auto* srcLight = dynamic_cast<StagePointLight*>(source)) {
			if (auto* dstLight = dynamic_cast<StagePointLight*>(created)) {
				dstLight->SetLight(srcLight->GetLightColor(), srcLight->GetLightOffset(),
					srcLight->GetLightIntensity(), srcLight->GetLightRadius(), srcLight->GetLightDecay());
			}
			return;
		}
		if (auto* srcProp = dynamic_cast<Prop*>(source)) {
			if (auto* dstProp = dynamic_cast<Prop*>(created)) {
				if (srcProp->HasLight()) {
					dstProp->SetLightEnabled(true);
					dstProp->SetLight(srcProp->GetLightColor(), srcProp->GetLightOffset(),
						srcProp->GetLightIntensity(), srcProp->GetLightRadius(), srcProp->GetLightDecay());
				}
			}
			return;
		}
		if (auto* srcEvent = dynamic_cast<BaseEvent*>(source)) {
			if (auto* dstEvent = dynamic_cast<BaseEvent*>(created)) {
				dstEvent->SetTargetNames(srcEvent->GetTargetNames());
			}
		}
	});
}

#endif // _DEBUG
