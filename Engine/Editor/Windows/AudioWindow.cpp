#include "AudioWindow.h"
#ifdef _DEBUG

#include "Audio/Audio.h"
#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"

#include <algorithm>
#include <filesystem>
#include <imgui/imgui.h>
#include <string>
#include <vector>

namespace {

// Resource/sound にある .wav（拡張子なしの名前。Audio の API がこの形を取る）
std::vector<std::string> g_diskSounds;
bool g_diskSoundsScanned = false;

// 試聴中のリソース番号。停止に使う
int g_playingHandle = -1;
std::string g_playingName;
bool g_loop = false;
float g_volume = 1.0f;

void ScanSounds()
{
	g_diskSounds.clear();

	const std::filesystem::path root = "Resource/sound";
	std::error_code ec;
	if (std::filesystem::is_directory(root, ec)) {
		for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
			if (!entry.is_regular_file()) {
				continue;
			}
			const std::filesystem::path& path = entry.path();
			if (path.extension() == ".wav" || path.extension() == ".WAV") {
				// Audio::SoundLoadWave は "resource/sound/<名前>.wav" を自分で組み立てるので、
				// ここでは拡張子を落とした名前だけ持つ
				g_diskSounds.push_back(path.stem().string());
			}
		}
	}
	std::sort(g_diskSounds.begin(), g_diskSounds.end());
	g_diskSoundsScanned = true;
}

} // namespace

void Editor::DrawAudioWindow()
{
	if (!EditorWindow::Begin("Audio", EditorWindow::Category::kEngine, 0, false)) {
		return;
	}

	Audio* audio = Ctx().audio;
	if (!audio) {
		ImGui::TextDisabled("EditorContext がまだ設定されていません");
		EditorWindow::End();
		return;
	}

	if (!g_diskSoundsScanned) {
		ScanSounds();
	}

	if (ImGui::Button("再スキャン")) {
		ScanSounds();
	}
	ImGui::SameLine();
	ImGui::Checkbox("ループ", &g_loop);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::SliderFloat("音量", &g_volume, 0.0f, 1.0f) && g_playingHandle >= 0) {
		audio->SetBGMVolume(g_playingHandle, g_volume);
	}

	if (g_playingHandle >= 0) {
		ImGui::TextDisabled("再生中: %s", g_playingName.c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("停止")) {
			audio->StopBGM(g_playingHandle);
			g_playingHandle = -1;
			g_playingName.clear();
		}
	}

	ImGui::Separator();

	// GetSoundData() は波形バッファごとコピーするので、こちらの const 参照版を使う
	const auto& loaded = audio->GetSoundDataMap();
	ImGui::TextDisabled("ディスク上 %zu 件 / 読み込み済み %zu 件", g_diskSounds.size(), loaded.size());

	if (!ImGui::BeginTable("##sounds", 4,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
		EditorWindow::End();
		return;
	}
	ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("状態", ImGuiTableColumnFlags_WidthFixed, 90.0f);
	ImGui::TableSetupColumn("サイズ", ImGuiTableColumnFlags_WidthFixed, 80.0f);
	ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 130.0f);
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableHeadersRow();

	for (const std::string& name : g_diskSounds) {
		ImGui::PushID(name.c_str());
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(name.c_str());

		const auto it = loaded.find(name);
		const bool isLoaded = (it != loaded.end());

		ImGui::TableNextColumn();
		if (isLoaded) {
			ImGui::TextUnformatted("読み込み済み");
		} else {
			ImGui::TextDisabled("未読み込み");
		}

		ImGui::TableNextColumn();
		if (isLoaded) {
			ImGui::Text("%.1f KB", static_cast<float>(it->second.bufferSize) / 1024.0f);
		} else {
			ImGui::TextDisabled("-");
		}

		ImGui::TableNextColumn();
		if (!isLoaded) {
			if (ImGui::SmallButton("読み込む")) {
				audio->SoundLoadWave(name.c_str());
				EditorMenuBar::ShowToast("%s を読み込みました", name.c_str());
			}
		} else if (ImGui::SmallButton("再生")) {
			// 前の試聴が残っていたら止めてから鳴らす
			if (g_playingHandle >= 0) {
				audio->StopBGM(g_playingHandle);
			}
			g_playingHandle = audio->SoundPlayWave(name.c_str(), g_loop);
			g_playingName = name;
			if (g_playingHandle >= 0) {
				audio->SetBGMVolume(g_playingHandle, g_volume);
			}
		}

		ImGui::PopID();
	}

	ImGui::EndTable();
	EditorWindow::End();
}

#endif // _DEBUG
