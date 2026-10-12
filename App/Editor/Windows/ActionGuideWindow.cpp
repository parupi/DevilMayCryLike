#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorHost.h>

#include "ActionGuide/ActionGuideSettings.h"
#include "ActionGuide/ActionGuideSystem.h"
#include "ActionGuide/InputIconProvider.h"
#include "GameData/GameSettings.h"

#include <imgui/imgui.h>

#include <cstring>
#include <string>

namespace {

constexpr const char* kGroup = ActionGuideSettings::kGroupName;

GlobalVariables& Global() { return GlobalVariables::GetInstance(); }

// GlobalVariables の値を直接いじる。GameScene が毎フレーム Apply() で読み直すので、その場で画面に効く
void DragFloatParam(const char* label, const char* key, float speed, float min, float max, const char* format = "%.1f")
{
	ImGui::DragFloat(label, &Global().GetValueRef<float>(kGroup, key), speed, min, max, format);
}

void StringParam(const char* label, const char* key)
{
	std::string& value = Global().GetValueRef<std::string>(kGroup, key);
	char buffer[128] = {};
	strncpy_s(buffer, value.c_str(), sizeof(buffer) - 1);
	if (ImGui::InputText(label, buffer, sizeof(buffer))) {
		value = buffer;
	}
}

const char* AvailabilityLabel(ActionAvailability availability)
{
	switch (availability) {
	case ActionAvailability::Executable: return "Executable";
	case ActionAvailability::Buffered:   return "Buffered";
	case ActionAvailability::Preview:    return "Preview";
	default:                             return "Unavailable";
	}
}

ImVec4 AvailabilityColor(ActionAvailability availability)
{
	switch (availability) {
	case ActionAvailability::Executable: return { 0.55f, 0.95f, 0.55f, 1.0f };
	case ActionAvailability::Buffered:   return { 0.95f, 0.85f, 0.45f, 1.0f };
	case ActionAvailability::Preview:    return { 0.6f, 0.6f, 0.6f, 1.0f };
	default:                             return { 0.9f, 0.4f, 0.4f, 1.0f };
	}
}

// 入力の組み合わせを1行の文字にする（例: "RB + ↑ + Y [長押し]"）
std::string BindingText(const InputBinding& binding, bool usePad)
{
	ActionGuideSettings words;
	words.Apply();
	std::string text = InputIconProvider::PrefixText(binding, usePad) + InputIconProvider::Resolve(binding.action, usePad).text;
	const std::string gesture = InputIconProvider::GestureText(binding, words);
	if (!gesture.empty()) text += " [" + gesture + "]";
	return text;
}

void DrawLiveView()
{
	const ActionGuideSystem* system = ActionGuideSystem::GetCurrent();
	if (!system) {
		ImGui::TextDisabled("ゲーム中のシーンではないので、ガイドは動いていません");
		return;
	}

	const ActionGuideContext& context = system->GetLastContext();
	ImGui::Text("State : %s   Attack : %s%s", context.state.empty() ? "-" : context.state.c_str(),
		context.currentAttack.empty() ? "-" : context.currentAttack.c_str(), context.isCharging ? " (溜め中)" : "");
	ImGui::Text("接地 %s / ロックオン %s / 戦闘中 %s / カウンター受付 %s",
		context.isGrounded ? "○" : "-", context.isLockOn ? "○" : "-",
		context.isInBattle ? "○" : "-", context.counterWindowOpen ? "○" : "-");

	const char* hidden = system->GetHiddenReason();
	if (hidden[0] != '\0') {
		ImGui::TextColored({ 0.9f, 0.6f, 0.4f, 1.0f }, "非表示 : %s", hidden);
		return;
	}

	const bool usePad = InputIconProvider::UsePad();
	const ActionGuideViewModel& viewModel = system->GetViewModel();
	if (ImGui::BeginTable("##ActionGuideLive", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
		ImGui::TableSetupColumn("可否");
		ImGui::TableSetupColumn("入力");
		ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("優先度");
		ImGui::TableSetupColumn("ID");
		ImGui::TableHeadersRow();
		for (const ActionGuideEntry& entry : viewModel.entries) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextColored(AvailabilityColor(entry.availability), "%s%s",
				AvailabilityLabel(entry.availability), entry.queued ? " ●" : "");
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(BindingText(entry.binding, usePad).c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(entry.displayName.c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%d", entry.priority);
			ImGui::TableNextColumn();
			ImGui::Text("%u", entry.id);
		}
		ImGui::EndTable();
	}
	if (viewModel.entries.empty()) {
		ImGui::TextDisabled("(今出せる行動はありません)");
	}
}

} // namespace

void AppEditor::DrawActionGuideWindow()
{
	if (!EditorWindow::Begin("Action Guide", EditorWindow::Category::kGame)) {
		return;
	}

	// ゲームのシーンに入る前でも触れるよう、項目が無ければここで登録する
	if (!Global().HasItem(kGroup, "MaxEntries")) {
		ActionGuideSettings settings;
		settings.RegisterAndLoad();
	}

	// ── 今の表示 ──
	if (ImGui::CollapsingHeader("今の表示（デバッグ）", ImGuiTreeNodeFlags_DefaultOpen)) {
		DrawLiveView();
	}

	// ── いつ出すか（OPTION と同じ値） ──
	{
		GameSettings& gameSettings = GameSettings::GetInstance();
		int32_t mode = gameSettings.GetActionGuideMode();
		const char* modes[] = { "OFF", "戦闘中のみ", "常に" };
		if (ImGui::Combo("表示モード（OPTION と共通）", &mode, modes, IM_ARRAYSIZE(modes))) {
			gameSettings.SetActionGuideMode(mode);
			gameSettings.Save();
		}
	}

	if (ImGui::CollapsingHeader("中身", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderInt("最大表示件数##MaxEntries", &Global().GetValueRef<int32_t>(kGroup, "MaxEntries"), 1, ActionGuideSettings::kMaxRows);
		ImGui::Checkbox("少し待つと出る派生を薄く出す (Preview)##ShowPreview", &Global().GetValueRef<bool>(kGroup, "ShowPreview"));
		ImGui::Checkbox("回避を出す##ShowDodge", &Global().GetValueRef<bool>(kGroup, "ShowDodge"));
		ImGui::Checkbox("ジャンプを出す##ShowJump", &Global().GetValueRef<bool>(kGroup, "ShowJump"));
		ImGui::Checkbox("ロックオンを出す（戦闘中・未ロック時）##ShowLockOn", &Global().GetValueRef<bool>(kGroup, "ShowLockOn"));

		ImGui::SeparatorText("優先度（大きいほど上。今押せるものは常に Preview より上）");
		for (int32_t i = 0; i < kActionGuideCategoryCount; ++i) {
			const auto category = static_cast<ActionGuideCategory>(i);
			const std::string label = std::string(ActionGuideSettings::CategoryLabel(category)) + "##" + ActionGuideSettings::PriorityKey(category);
			ImGui::DragInt(label.c_str(), &Global().GetValueRef<int32_t>(kGroup, ActionGuideSettings::PriorityKey(category)), 0.5f, -100, 200);
		}
	}

	if (ImGui::CollapsingHeader("名前・言葉")) {
		ImGui::TextDisabled("攻撃の名前は Attack Editor の「Guide Name」で変える");
		StringParam("ジャンプ##NameJump", "NameJump");
		StringParam("回避##NameDodge", "NameDodge");
		StringParam("ロックオン##NameLockOn", "NameLockOn");
		StringParam("長押し##TextHold", "TextHold");
		StringParam("離す##TextRelease", "TextRelease");
		StringParam("少し待って##TextWait", "TextWait");
	}

	if (ImGui::CollapsingHeader("配置")) {
		DragFloatParam("アイコンの中心 X##IconX", "IconX", 1.0f, 0.0f, 1280.0f);
		DragFloatParam("一番下の行の Y##BottomY", "BottomY", 1.0f, 0.0f, 720.0f);
		DragFloatParam("チュートリアルの部屋で持ち上げる量##TutorialRoomRaise", "TutorialRoomRaise", 1.0f, 0.0f, 300.0f);
		DragFloatParam("行の間隔##RowSpacing", "RowSpacing", 0.5f, 10.0f, 100.0f);
		DragFloatParam("アイコンの大きさ##IconSize", "IconSize", 0.5f, 8.0f, 80.0f);
		DragFloatParam("名前の文字の大きさ##FontSize", "FontSize", 0.5f, 8.0f, 60.0f);
		DragFloatParam("押し方の文字の大きさ##GestureFontSize", "GestureFontSize", 0.5f, 8.0f, 60.0f);
		DragFloatParam("アイコンと文字の間##TextGap", "TextGap", 0.5f, 0.0f, 60.0f);
		DragFloatParam("帯の幅##BackdropWidth", "BackdropWidth", 1.0f, 0.0f, 800.0f);
		DragFloatParam("帯の濃さ##BackdropAlpha", "BackdropAlpha", 0.01f, 0.0f, 1.0f, "%.2f");
	}

	if (ImGui::CollapsingHeader("見え方")) {
		DragFloatParam("通常の濃さ##NormalAlpha", "NormalAlpha", 0.01f, 0.0f, 1.0f, "%.2f");
		DragFloatParam("Preview の濃さ##PreviewAlpha", "PreviewAlpha", 0.01f, 0.0f, 1.0f, "%.2f");
		DragFloatParam("切り替えのフェード##FadeTime", "FadeTime", 0.005f, 0.0f, 1.0f, "%.3f 秒");
		ImGui::SetItemTooltip("仕様書の目安は 0.10〜0.18 秒");
		DragFloatParam("フェードの始まりの濃さ##FadeStartAlpha", "FadeStartAlpha", 0.01f, 0.0f, 1.0f, "%.2f");
		ImGui::SetItemTooltip("0 から始めると、短い受付窓で表示が遅れて情報が隠れる");
		DragFloatParam("横すべりの量##SlideOffset", "SlideOffset", 0.5f, 0.0f, 100.0f);
		DragFloatParam("先行入力済みの印の大きさ##QueuedMarkSize", "QueuedMarkSize", 0.5f, 0.0f, 40.0f);
	}

	ImGui::Separator();
	if (ImGui::Button("Save##ActionGuide")) {
		ActionGuideSettings::Save();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Resource/GlobalVariables/%s/%s.json", ActionGuideSettings::kDirectoryName, kGroup);

	EditorWindow::End();
}

#endif // _DEBUG
