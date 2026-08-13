#include "AnimationWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"

#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "World3D/Object/Model/SkinnedModel.h"
#include "World3D/Object/Model/Animation/SkinnedInstance.h"
#include "World3D/Object/Model/Animation/AnimationPlayer.h"
#include "World3D/Object/Model/Animation/AnimationClipSet.h"
#include "World3D/Object/Model/Animation/Skeleton.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <string>
#include <vector>

namespace {

// シーンにいるスキンモデル1体ぶんの参照
struct SkinnedEntry {
	std::string label;      // "オブジェクト名 / レンダラー名"
	SkinnedInstance* instance = nullptr;
};

std::string g_selectedLabel;

// 再生設定（ウィンドウ側の入力欄。インスタンスには Play したときだけ渡る）
float g_blendTime = 0.15f;
bool g_playLoop = true;
bool g_forceRestart = false;

// レイヤー
char g_layerMaskRoot[64] = "Torso";
float g_layerBlendTime = 0.2f;
bool g_layerLoop = true;

// ルートモーション
char g_rootMotionJoint[64] = "";

// イベント追加欄
char g_newEventTag[64] = "hit_start";

// スクラブ中は再生を止めておきたいので、直前の速度を覚えておく
bool g_paused = false;
float g_speedBeforePause = 1.0f;

std::vector<SkinnedEntry> CollectSkinnedInstances()
{
	std::vector<SkinnedEntry> entries;

	Object3dManager* objects = Editor::Ctx().object3dManager;
	if (!objects) return entries;

	for (Object3d* object : objects->GetAllObject()) {
		if (!object) continue;
		for (BaseRenderer* renderer : object->GetRenderers()) {
			if (!renderer) continue;
			SkinnedInstance* instance = renderer->GetSkinnedInstance();
			if (!instance) continue;

			SkinnedEntry entry;
			entry.label = object->name_ + " / " + renderer->name_;
			entry.instance = instance;
			entries.push_back(std::move(entry));
		}
	}
	return entries;
}

void DrawPlaybackSection(SkinnedInstance* instance, SkinnedModel* asset)
{
	AnimationPlayer* player = instance->GetPlayer();
	const AnimationClipSet* clips = asset->GetClipSet();

	const std::string& currentName = player->GetCurrentClipName();
	const float duration = player->GetDuration();

	ImGui::Text("再生中: %s", currentName.empty() ? "(なし)" : currentName.c_str());
	if (player->IsBlending()) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "(ブレンド中)");
	}
	if (player->IsFinished()) {
		ImGui::SameLine();
		ImGui::TextDisabled("(終了)");
	}

	// --- 再生位置のスクラブ ---
	float time = player->GetTime();
	ImGui::SetNextItemWidth(-120.0f);
	if (ImGui::SliderFloat("##time", &time, 0.0f, (duration > 0.0f) ? duration : 1.0f, "%.3f 秒")) {
		player->SetTime(time);
		// つまんでいる間も絵を更新したいので、その場でポーズを作り直す
		instance->Update(0.0f);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("/ %.3f", duration);

	// --- 一時停止 ---
	if (ImGui::Button(g_paused ? "再開" : "一時停止")) {
		if (g_paused) {
			player->SetSpeed(g_speedBeforePause);
			g_paused = false;
		} else {
			g_speedBeforePause = player->GetSpeed();
			player->SetSpeed(0.0f);
			g_paused = true;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("頭出し")) {
		player->SetTime(0.0f);
		instance->Update(0.0f);
	}

	float speed = player->GetSpeed();
	ImGui::SetNextItemWidth(160.0f);
	if (ImGui::DragFloat("速度", &speed, 0.01f, -3.0f, 3.0f)) {
		player->SetSpeed(speed);
		if (g_paused) g_speedBeforePause = speed;
	}

	bool loop = player->IsLoop();
	if (ImGui::Checkbox("ループ", &loop)) {
		player->SetLoop(loop);
	}

	ImGui::Separator();

	// --- クリップ一覧 ---
	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragFloat("ブレンド時間", &g_blendTime, 0.01f, 0.0f, 2.0f);
	ImGui::SameLine();
	ImGui::Checkbox("ループ再生", &g_playLoop);
	ImGui::SameLine();
	ImGui::Checkbox("頭から出し直す", &g_forceRestart);

	const std::vector<std::string> clipNames = clips->GetClipNames();
	if (clipNames.empty()) {
		ImGui::TextDisabled("(クリップがありません)");
		return;
	}

	if (ImGui::BeginChild("##clips", ImVec2(0.0f, 150.0f), ImGuiChildFlags_Borders)) {
		for (const std::string& name : clipNames) {
			ImGui::PushID(name.c_str());
			const bool isCurrent = (name == currentName);
			if (ImGui::Selectable(name.c_str(), isCurrent)) {
				player->Play(name, g_playLoop, g_blendTime, g_forceRestart);
				// 一時停止したまま別クリップへ行くと止まって見えるので解除しておく
				if (g_paused) {
					player->SetSpeed(g_speedBeforePause);
					g_paused = false;
				}
			}
			const AnimationData* clip = clips->Find(name);
			if (clip) {
				ImGui::SameLine();
				ImGui::TextDisabled("  %.2f秒  イベント%zu", clip->duration, clip->events.size());
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
}

void DrawLayerSection(SkinnedInstance* instance, SkinnedModel* asset)
{
	AnimationPlayer* player = instance->GetPlayer();
	const AnimationClipSet* clips = asset->GetClipSet();

	if (player->IsLayerActive()) {
		ImGui::Text("レイヤー: %s", player->GetLayerClipName().c_str());
		ImGui::SameLine();
		if (ImGui::Button("外す")) {
			player->StopLayer(g_layerBlendTime);
		}
	} else {
		ImGui::TextDisabled("レイヤーなし");
	}

	ImGui::SetNextItemWidth(160.0f);
	ImGui::InputText("マスク基点ジョイント", g_layerMaskRoot, sizeof(g_layerMaskRoot));
	ImGui::SameLine();
	ImGui::TextDisabled("(このジョイントと子孫にだけ効く)");

	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragFloat("ブレンド時間##layer", &g_layerBlendTime, 0.01f, 0.0f, 2.0f);
	ImGui::SameLine();
	ImGui::Checkbox("ループ##layer", &g_layerLoop);

	// マスク基点が実在しないと全身に効いてしまうので、先に知らせる
	const bool maskExists = instance->GetSkeleton()->FindJoint(g_layerMaskRoot) != nullptr;
	if (!maskExists) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "そのジョイントはありません");
	}

	const std::vector<std::string> clipNames = clips->GetClipNames();
	ImGui::BeginDisabled(!maskExists);
	if (ImGui::BeginCombo("重ねるクリップ", "選ぶ...")) {
		for (const std::string& name : clipNames) {
			if (ImGui::Selectable(name.c_str())) {
				player->PlayLayer(name, g_layerMaskRoot, g_layerLoop, g_layerBlendTime);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
}

void DrawRootMotionSection(SkinnedInstance* instance)
{
	AnimationPlayer* player = instance->GetPlayer();

	ImGui::SetNextItemWidth(160.0f);
	if (ImGui::InputText("抽出ジョイント", g_rootMotionJoint, sizeof(g_rootMotionJoint),
		ImGuiInputTextFlags_EnterReturnsTrue)) {
		player->SetRootMotionJoint(g_rootMotionJoint);
	}
	ImGui::SameLine();
	if (ImGui::Button("適用")) {
		player->SetRootMotionJoint(g_rootMotionJoint);
	}
	ImGui::SameLine();
	if (ImGui::Button("無効化")) {
		g_rootMotionJoint[0] = '\0';
		player->SetRootMotionJoint("");
	}

	if (!player->IsRootMotionEnabled()) {
		ImGui::TextDisabled("無効（ポーズから何も抜き取らない）");
		return;
	}

	// ここで Consume すると本来の受け取り側から移動量を奪ってしまうので、表示はしない。
	// 代わりに「今どのジョイントから抜いているか」だけ出す
	ImGui::TextDisabled("有効。移動量はゲーム側の ConsumeRootMotion() が受け取る");
	ImGui::TextDisabled("※このクリップに移動キーが無ければ、抜き取れる量は 0 のまま");
}

void DrawEventSection(SkinnedInstance* instance, SkinnedModel* asset)
{
	AnimationPlayer* player = instance->GetPlayer();
	AnimationClipSet* clips = asset->GetClipSetMutable();

	const std::string clipName = player->GetCurrentClipName();
	if (clipName.empty()) {
		ImGui::TextDisabled("(再生中のクリップがありません)");
		return;
	}

	ImGui::Text("対象クリップ: %s", clipName.c_str());

	std::vector<AnimationEvent>* events = clips->GetEventsMutable(clipName);
	if (!events) {
		ImGui::TextDisabled("(クリップが見つかりません)");
		return;
	}

	const float duration = player->GetDuration();

	// --- 追加 ---
	ImGui::SetNextItemWidth(160.0f);
	ImGui::InputText("タグ", g_newEventTag, sizeof(g_newEventTag));
	ImGui::SameLine();
	if (ImGui::Button("今の再生位置に追加")) {
		clips->AddEvent(clipName, player->GetTime(), g_newEventTag);
	}

	// --- 一覧 ---
	if (events->empty()) {
		ImGui::TextDisabled("(イベントなし)");
	} else {
		int removeIndex = -1;
		bool needSort = false;

		for (size_t i = 0; i < events->size(); ++i) {
			ImGui::PushID(static_cast<int>(i));

			float time = (*events)[i].time;
			ImGui::SetNextItemWidth(120.0f);
			if (ImGui::DragFloat("##t", &time, 0.005f, 0.0f, (duration > 0.0f) ? duration : 1.0f, "%.3f 秒")) {
				(*events)[i].time = time;
				needSort = true;
			}
			ImGui::SameLine();
			ImGui::TextUnformatted((*events)[i].tag.c_str());

			ImGui::SameLine();
			if (ImGui::SmallButton("ここへ移動")) {
				player->SetTime((*events)[i].time);
				instance->Update(0.0f);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("削除")) {
				removeIndex = static_cast<int>(i);
			}
			ImGui::PopID();
		}

		// ドラッグ中に並べ替えると掴んでいる行が入れ替わるので、1フレーム分まとめて後始末する
		if (needSort && !ImGui::IsAnyItemActive()) {
			clips->SortEvents(clipName);
		}
		if (removeIndex >= 0) {
			clips->RemoveEvent(clipName, static_cast<size_t>(removeIndex));
		}
	}

	ImGui::Separator();
	if (ImGui::Button("このモデルの .anim.json に保存")) {
		if (clips->SaveEvents(asset->GetModelName())) {
			EditorMenuBar::ShowToast("%s.anim.json に保存しました", asset->GetModelName().c_str());
		} else {
			EditorMenuBar::ShowToast("保存に失敗しました");
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Resource/Models/%s/%s.anim.json",
		asset->GetModelName().c_str(), asset->GetModelName().c_str());
}

void DrawSkeletonSection(SkinnedInstance* instance)
{
	const SkeletonData& skeleton = instance->GetSkeleton()->GetSkeletonData();

	ImGui::TextDisabled("ジョイント数 %zu（ボーン追従やマスクに使う名前はここで探す）", skeleton.joints.size());

	if (ImGui::BeginChild("##joints", ImVec2(0.0f, 200.0f), ImGuiChildFlags_Borders)) {
		for (const Joint& joint : skeleton.joints) {
			ImGui::PushID(joint.index);
			// 階層が分かるように親の深さぶん字下げする
			int depth = 0;
			std::optional<int32_t> parent = joint.parent;
			while (parent.has_value() && depth < 32) {
				parent = skeleton.joints[*parent].parent;
				++depth;
			}
			ImGui::Indent(depth * 10.0f);
			ImGui::TextUnformatted(joint.name.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("コピー")) {
				ImGui::SetClipboardText(joint.name.c_str());
			}
			ImGui::Unindent(depth * 10.0f);
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
}

} // namespace

void Editor::DrawAnimationWindow()
{
	// 初回だけ。以降は imgui.ini に保存された大きさが使われる
	ImGui::SetNextWindowSize(ImVec2(460.0f, 720.0f), ImGuiCond_FirstUseEver);
	if (!EditorWindow::Begin("Animation", EditorWindow::Category::kCharacter)) {
		return;
	}

	const std::vector<SkinnedEntry> entries = CollectSkinnedInstances();
	if (entries.empty()) {
		ImGui::TextDisabled("シーンにスキンモデルがありません");
		ImGui::TextDisabled("（ModelManager::LoadSkinnedModel したモデルを ModelRenderer で置くと出ます）");
		EditorWindow::End();
		return;
	}

	// 選択が消えていたら先頭へ寄せる
	const SkinnedEntry* selected = nullptr;
	for (const SkinnedEntry& entry : entries) {
		if (entry.label == g_selectedLabel) {
			selected = &entry;
			break;
		}
	}
	if (!selected) {
		g_selectedLabel = entries.front().label;
		selected = &entries.front();
	}

	if (ImGui::BeginCombo("対象", g_selectedLabel.c_str())) {
		for (const SkinnedEntry& entry : entries) {
			if (ImGui::Selectable(entry.label.c_str(), entry.label == g_selectedLabel)) {
				g_selectedLabel = entry.label;
			}
		}
		ImGui::EndCombo();
	}

	SkinnedInstance* instance = selected->instance;
	SkinnedModel* asset = instance ? instance->GetAsset() : nullptr;
	if (!instance || !asset) {
		ImGui::TextDisabled("インスタンスが取れませんでした");
		EditorWindow::End();
		return;
	}

	ImGui::TextDisabled("モデル: %s", asset->GetModelName().c_str());
	ImGui::Separator();

	if (ImGui::CollapsingHeader("再生", ImGuiTreeNodeFlags_DefaultOpen)) {
		DrawPlaybackSection(instance, asset);
	}
	if (ImGui::CollapsingHeader("上半身レイヤー")) {
		DrawLayerSection(instance, asset);
	}
	if (ImGui::CollapsingHeader("ルートモーション")) {
		DrawRootMotionSection(instance);
	}
	if (ImGui::CollapsingHeader("アニメーションイベント", ImGuiTreeNodeFlags_DefaultOpen)) {
		DrawEventSection(instance, asset);
	}
	if (ImGui::CollapsingHeader("ジョイント")) {
		DrawSkeletonSection(instance);
	}

	EditorWindow::End();
}

#endif // _DEBUG
