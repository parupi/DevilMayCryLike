#include "SoundEditorWindow.h"
#ifdef _DEBUG

#include "Audio/Audio.h"
#include "Audio/SE/SoundAssetLibrary.h"
#include "Audio/SE/SoundDefinition.h"
#include "Audio/SE/SoundPresets.h"
#include "Audio/SE/SoundSynthesizer.h"
#include "Audio/SoundManager.h"
#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <imgui/imgui.h>
#include <string>
#include <vector>

namespace {

// 試聴専用の登録名。編集中の音を本来の名前で登録すると、
// ゲームが同じ名前を鳴らしたときに未保存の内容が混ざってしまう
const char* const kPreviewName = "__SEEditorPreview";

// WAV の書き出し先。Resource/sound を直接上書きしない（既存素材を潰さないため）
const char* const kExportDirectory = "Resource/Sounds/Export/";

// 編集中の SE
SoundDefinition g_definition = SoundPresets::CreateEmpty("NewSound");
// 開いているファイル名。空なら「まだ保存していない新規」
std::string g_openedName;
bool g_unsaved = false;

// 焼き直しが要るか。スライダーを離した時にだけ焼く
bool g_needsRender = true;
AudioBuffer g_preview;
float g_lastRenderMs = 0.0f;
std::vector<float> g_spectrum;

// 試聴
int g_playHandle = -1;
double g_playStartedAt = 0.0;
float g_previewVolume = 0.8f;
bool g_autoPlay = false;

// .sound の一覧（起動時とファイル操作のたびに取り直す）
std::vector<std::string> g_soundNames;
bool g_soundNamesScanned = false;

int g_presetIndex = 0;

void RefreshSoundList()
{
	g_soundNames = SoundFile::ListNames();
	g_soundNamesScanned = true;
}

void MarkDirty()
{
	g_needsRender = true;
	g_unsaved = true;
}

// ───────────────────────────── 値を触る小物 ─────────────────────────────
// どれも「変わったら焼き直す」を付けて回るだけ。戻り値は変更されたか

bool DragFloatDirty(const char* label, float* value, float speed, float min, float max,
	const char* format = "%.3f", ImGuiSliderFlags flags = 0)
{
	if (ImGui::DragFloat(label, value, speed, min, max, format, flags)) {
		MarkDirty();
		return true;
	}
	return false;
}

bool SliderFloatDirty(const char* label, float* value, float min, float max,
	const char* format = "%.3f", ImGuiSliderFlags flags = 0)
{
	if (ImGui::SliderFloat(label, value, min, max, format, flags)) {
		MarkDirty();
		return true;
	}
	return false;
}

bool CheckboxDirty(const char* label, bool* value)
{
	if (ImGui::Checkbox(label, value)) {
		MarkDirty();
		return true;
	}
	return false;
}

/// <summary>周波数は対数スライダーで触る。線形だと低域がほとんど動かせない</summary>
bool FrequencySlider(const char* label, float* value, float min = 20.0f, float max = 18000.0f)
{
	return SliderFloatDirty(label, value, min, max, "%.0f Hz", ImGuiSliderFlags_Logarithmic);
}

template <typename EnumType>
bool EnumCombo(const char* label, EnumType* value, const char* const* names, int count)
{
	int index = static_cast<int>(*value);
	if (ImGui::Combo(label, &index, names, count)) {
		*value = static_cast<EnumType>(index);
		MarkDirty();
		return true;
	}
	return false;
}

// ───────────────────────────── 合成と試聴 ─────────────────────────────

void RenderPreview()
{
	const auto begin = std::chrono::high_resolution_clock::now();
	g_preview = SoundAssets::RenderAndRegister(g_definition, kPreviewName);
	const auto end = std::chrono::high_resolution_clock::now();
	g_lastRenderMs = std::chrono::duration<float, std::milli>(end - begin).count();

	// スペクトラムは音の頭（＝いちばん特徴が出るところ）を見る
	g_spectrum.clear();
	if (!g_preview.IsEmpty()) {
		const size_t fftSize = 2048;
		std::vector<float> mono;
		mono.reserve(fftSize);
		const size_t frames = (std::min)(g_preview.FrameCount(), fftSize);
		for (size_t i = 0; i < frames; ++i) {
			float sum = 0.0f;
			for (int ch = 0; ch < g_preview.channels; ++ch) {
				sum += g_preview.Sample(i, ch);
			}
			mono.push_back(sum / static_cast<float>(g_preview.channels));
		}
		g_spectrum = SoundDSP::ComputeSpectrum(mono.data(), mono.size(), fftSize);
	}

	g_needsRender = false;
}

/// <summary>
/// 試聴が今も鳴っているか。
/// ボイスのスロットは使い回されるので、番号だけでなく中身の名前も確かめる
/// （焼き直しで試聴が止まったあと、同じ番号をゲームの SE が拾っていることがある）
/// </summary>
bool IsPreviewPlaying()
{
	if (g_playHandle < 0) { return false; }
	Audio& audio = Audio::GetInstance();
	return audio.GetVoiceSoundName(g_playHandle) == kPreviewName && audio.IsPlaying(g_playHandle);
}

void StopPreview()
{
	if (g_playHandle < 0) { return; }
	Audio& audio = Audio::GetInstance();
	if (audio.GetVoiceSoundName(g_playHandle) == kPreviewName) {
		audio.StopBGM(g_playHandle);
	}
	g_playHandle = -1;
}

void PlayPreview()
{
	if (g_needsRender) { RenderPreview(); }
	if (g_preview.IsEmpty()) { return; }

	StopPreview();
	Audio& audio = Audio::GetInstance();
	g_playHandle = audio.SoundPlayWave(kPreviewName, false);
	if (g_playHandle >= 0) {
		audio.SetBGMVolume(g_playHandle, g_previewVolume);
		g_playStartedAt = ImGui::GetTime();
	}
}

/// <summary>レイヤー1本だけを鳴らす。どの層がどの音か確かめるのに使う</summary>
void PlayLayerSolo(const SELayer& layer)
{
	SoundDefinition solo = g_definition;
	solo.layers.clear();

	SELayer copy = layer;
	copy.enabled = true;
	copy.startDelay = 0.0f; // 単体で聞くのに待たされても意味がない
	solo.layers.push_back(copy);

	StopPreview();
	SoundAssets::RenderAndRegister(solo, kPreviewName);

	Audio& audio = Audio::GetInstance();
	g_playHandle = audio.SoundPlayWave(kPreviewName, false);
	if (g_playHandle >= 0) {
		audio.SetBGMVolume(g_playHandle, g_previewVolume);
		g_playStartedAt = ImGui::GetTime();
	}
	// 試聴の登録名を書き換えたので、全体を鳴らす前に焼き直す
	g_needsRender = true;
}

// ───────────────────────────── 表示 ─────────────────────────────

/// <summary>波形表示（設計書 Phase 7「波形プレビュー」）</summary>
void DrawWaveform(float height)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float width = (std::max)(ImGui::GetContentRegionAvail().x, 32.0f);
	const ImVec2 size(width, height);

	ImGui::InvisibleButton("##waveform", size);

	drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(18, 18, 22, 255));
	drawList->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(70, 70, 80, 255));

	const float centerY = origin.y + size.y * 0.5f;
	drawList->AddLine(ImVec2(origin.x, centerY), ImVec2(origin.x + size.x, centerY), IM_COL32(60, 60, 70, 255));

	const size_t frames = g_preview.FrameCount();
	if (frames == 0) {
		const char* message = "波形がありません（レイヤーを追加してください）";
		drawList->AddText(ImVec2(origin.x + 8.0f, centerY - 8.0f), IM_COL32(150, 150, 160, 255), message);
		return;
	}

	// 1ピクセルの列に何サンプルも入るので、最小値と最大値を縦線で結ぶ。
	// 間引いて点を打つやり方だと、短い立ち上がりが列ごと消えてしまう
	const int columns = static_cast<int>(size.x);
	const float halfHeight = size.y * 0.5f - 2.0f;
	for (int x = 0; x < columns; ++x) {
		const size_t from = static_cast<size_t>(static_cast<double>(x) / columns * frames);
		size_t to = static_cast<size_t>(static_cast<double>(x + 1) / columns * frames);
		if (to <= from) { to = from + 1; }
		if (from >= frames) { break; }

		float lowest = 1.0f;
		float highest = -1.0f;
		for (size_t i = from; i < to && i < frames; ++i) {
			for (int ch = 0; ch < g_preview.channels; ++ch) {
				const float value = g_preview.Sample(i, ch);
				if (value < lowest) { lowest = value; }
				if (value > highest) { highest = value; }
			}
		}
		if (lowest > highest) { continue; }

		const float screenX = origin.x + static_cast<float>(x) + 0.5f;
		drawList->AddLine(
			ImVec2(screenX, centerY - highest * halfHeight),
			ImVec2(screenX, centerY - lowest * halfHeight),
			IM_COL32(110, 200, 255, 255));
	}

	// 再生位置。実時間で進めるのでポーズ中でも動く
	if (IsPreviewPlaying()) {
		const float elapsed = static_cast<float>(ImGui::GetTime() - g_playStartedAt);
		const float duration = g_preview.DurationSeconds();
		if (duration > 0.0f && elapsed <= duration) {
			const float x = origin.x + size.x * (elapsed / duration);
			drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + size.y), IM_COL32(255, 200, 80, 220), 1.5f);
		}
	}

	// 目盛り（0.1秒ごと）
	const float duration = g_preview.DurationSeconds();
	if (duration > 0.0f) {
		for (float t = 0.1f; t < duration; t += 0.1f) {
			const float x = origin.x + size.x * (t / duration);
			drawList->AddLine(ImVec2(x, origin.y + size.y - 5.0f), ImVec2(x, origin.y + size.y),
				IM_COL32(90, 90, 100, 255));
		}
	}
}

/// <summary>スペクトラム表示（設計書 Phase 7「スペクトラム表示」）</summary>
void DrawSpectrum(float height)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float width = (std::max)(ImGui::GetContentRegionAvail().x, 32.0f);
	const ImVec2 size(width, height);

	ImGui::InvisibleButton("##spectrum", size);

	drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(18, 18, 22, 255));
	drawList->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(70, 70, 80, 255));

	if (g_spectrum.empty() || g_preview.sampleRate <= 0) { return; }

	// 耳の感じ方に合わせて、横は周波数の対数、縦は dB で並べる。
	// 線形のままだと低域が左端に潰れて何も読み取れない
	constexpr int kBarCount = 72;
	constexpr float kMinHz = 40.0f;
	constexpr float kFloorDb = -70.0f;

	const float nyquist = static_cast<float>(g_preview.sampleRate) * 0.5f;
	const float logMin = std::log10(kMinHz);
	const float logMax = std::log10(nyquist);
	const float binWidth = nyquist / static_cast<float>(g_spectrum.size());
	const float barWidth = size.x / static_cast<float>(kBarCount);

	for (int i = 0; i < kBarCount; ++i) {
		const float fromHz = std::pow(10.0f, logMin + (logMax - logMin) * (static_cast<float>(i) / kBarCount));
		const float toHz = std::pow(10.0f, logMin + (logMax - logMin) * (static_cast<float>(i + 1) / kBarCount));

		size_t fromBin = static_cast<size_t>(fromHz / binWidth);
		size_t toBin = static_cast<size_t>(toHz / binWidth);
		if (toBin <= fromBin) { toBin = fromBin + 1; }

		float peak = 0.0f;
		for (size_t bin = fromBin; bin < toBin && bin < g_spectrum.size(); ++bin) {
			if (g_spectrum[bin] > peak) { peak = g_spectrum[bin]; }
		}

		const float db = (peak > 1.0e-6f) ? 20.0f * std::log10(peak) : kFloorDb;
		const float level = std::clamp((db - kFloorDb) / -kFloorDb, 0.0f, 1.0f);

		const float x0 = origin.x + barWidth * static_cast<float>(i) + 1.0f;
		const float x1 = origin.x + barWidth * static_cast<float>(i + 1) - 1.0f;
		const float y0 = origin.y + size.y * (1.0f - level);
		const float y1 = origin.y + size.y;
		if (level > 0.001f) {
			// 高い帯域ほど色を振って、どこが鳴っているか一目で分かるようにする
			const int red = static_cast<int>(80 + 175 * (static_cast<float>(i) / kBarCount));
			drawList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(red, 200 - red / 3, 255 - red / 2, 255));
		}
	}

	// 周波数の目印
	const float labels[] = { 100.0f, 1000.0f, 10000.0f };
	const char* labelTexts[] = { "100", "1k", "10k" };
	for (int i = 0; i < 3; ++i) {
		if (labels[i] >= nyquist) { continue; }
		const float ratio = (std::log10(labels[i]) - logMin) / (logMax - logMin);
		const float x = origin.x + size.x * ratio;
		drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + size.y), IM_COL32(70, 70, 80, 160));
		drawList->AddText(ImVec2(x + 2.0f, origin.y + 2.0f), IM_COL32(140, 140, 150, 255), labelTexts[i]);
	}
}

// ───────────────────────────── 各セクション ─────────────────────────────

void DrawFileList()
{
	if (!g_soundNamesScanned) {
		// 初回に .sound が1つも無ければプリセットを置いておく。
		// 空の一覧を見せられても、何から触ればいいのか分からないため
		if (SoundFile::ListNames().empty()) {
			SoundPresets::ExportAll(false);
		}
		RefreshSoundList();
	}

	ImGui::TextDisabled("Resource/Sounds");
	ImGui::Separator();

	if (ImGui::Button("新規", ImVec2(-FLT_MIN, 0.0f))) {
		g_definition = SoundPresets::CreateEmpty("NewSound");
		g_openedName.clear();
		g_unsaved = true;
		g_needsRender = true;
	}

	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::BeginCombo("##preset", "プリセットから作る")) {
		for (int i = 0; i < SoundPresets::Count(); ++i) {
			if (ImGui::Selectable(SoundPresets::GetName(i))) {
				g_definition = SoundPresets::Create(i);
				// 既存ファイルを開いたわけではないので、保存するまで新規扱い
				g_openedName.clear();
				g_unsaved = true;
				g_needsRender = true;
				g_presetIndex = i;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", SoundPresets::GetDescription(i));
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("プリセットを全部書き出す", ImVec2(-FLT_MIN, 0.0f))) {
		const int written = SoundPresets::ExportAll(false);
		RefreshSoundList();
		EditorMenuBar::ShowToast("プリセットを %d 件書き出しました（既存はそのまま）", written);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Resource/Sounds/ に .sound を作ります。\n"
			"既にある .sound と、同じ名前の .wav 素材があるもの（SwordSlash など）は触りません");
	}

	ImGui::Separator();

	if (ImGui::BeginChild("##soundlist", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing() * 2.0f))) {
		for (const std::string& name : g_soundNames) {
			const bool selected = (name == g_openedName);
			if (ImGui::Selectable(name.c_str(), selected)) {
				SoundDefinition loaded;
				if (SoundFile::Load(name, loaded)) {
					g_definition = std::move(loaded);
					g_openedName = name;
					g_unsaved = false;
					g_needsRender = true;
				} else {
					EditorMenuBar::ShowToast("%s を読み込めませんでした", name.c_str());
				}
			}
		}
	}
	ImGui::EndChild();

	ImGui::BeginDisabled(g_openedName.empty());
	if (ImGui::Button("複製", ImVec2(-FLT_MIN, 0.0f))) {
		g_definition.name = g_openedName + "_Copy";
		g_openedName.clear();
		g_unsaved = true;
	}
	if (ImGui::Button("削除", ImVec2(-FLT_MIN, 0.0f))) {
		ImGui::OpenPopup("削除の確認");
	}
	ImGui::EndDisabled();

	if (ImGui::BeginPopupModal("削除の確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("%s.sound を削除します。元に戻せません", g_openedName.c_str());
		if (ImGui::Button("削除する")) {
			if (SoundFile::Delete(g_openedName)) {
				SoundAssets::Invalidate(g_openedName);
				EditorMenuBar::ShowToast("%s を削除しました", g_openedName.c_str());
			}
			g_openedName.clear();
			RefreshSoundList();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("やめる")) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
}

void DrawTransport()
{
	char nameBuffer[128] = {};
	strncpy_s(nameBuffer, g_definition.name.c_str(), sizeof(nameBuffer) - 1);
	ImGui::SetNextItemWidth(220.0f);
	if (ImGui::InputText("名前", nameBuffer, sizeof(nameBuffer))) {
		g_definition.name = nameBuffer;
		g_unsaved = true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90.0f);
	if (ImGui::DragInt("優先度", &g_definition.priority, 1.0f, 0, 200)) {
		g_unsaved = true;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("同時発音が上限に達したとき、低いものから止まります\n例: ボス攻撃100 / ジャスト回避90 / プレイヤー攻撃80 / 足音20");
	}
	ImGui::SameLine();
	if (g_unsaved) {
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "未保存");
	} else {
		ImGui::TextDisabled("保存済み");
	}

	if (ImGui::Button("再生")) { PlayPreview(); }
	ImGui::SameLine();
	if (ImGui::Button("停止")) { StopPreview(); }
	ImGui::SameLine();
	ImGui::Checkbox("変更したら鳴らす", &g_autoPlay);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(130.0f);
	if (ImGui::SliderFloat("試聴音量", &g_previewVolume, 0.0f, 1.0f) && IsPreviewPlaying()) {
		Audio::GetInstance().SetBGMVolume(g_playHandle, g_previewVolume);
	}

	ImGui::BeginDisabled(g_definition.name.empty());
	if (ImGui::Button("保存")) {
		if (SoundFile::Save(g_definition)) {
			g_openedName = g_definition.name;
			g_unsaved = false;
			// 次に鳴らすとき新しい内容で焼き直させる
			SoundAssets::Invalidate(g_definition.name);
			RefreshSoundList();
			EditorMenuBar::ShowToast("%s.sound を保存しました", g_definition.name.c_str());
		} else {
			EditorMenuBar::ShowToast("保存に失敗しました");
		}
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Resource/Sounds/<名前>.sound へ書き出します。\n名前を変えてから押すと別ファイルになります");
	}
	ImGui::SameLine();
	if (ImGui::Button("WAV書き出し")) {
		if (g_needsRender) { RenderPreview(); }
		const std::string path = std::string(kExportDirectory) + g_definition.name + ".wav";
		if (g_preview.WriteWavFile(path)) {
			EditorMenuBar::ShowToast("%s へ書き出しました", path.c_str());
		} else {
			EditorMenuBar::ShowToast("WAV の書き出しに失敗しました");
		}
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s へ 16bit PCM で書き出します。\nResource/sound の既存素材は上書きしません", kExportDirectory);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("ゲームの名前で鳴らす")) {
		// 保存済みの内容を SoundManager 経由で鳴らして、実際の音量設定込みで確かめる
		SoundManager::GetInstance().PlaySE(g_definition.name);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("SoundManager::PlaySE(\"%s\") を呼びます。\n保存済みの .sound と、ゲーム側の SE 音量が反映されます",
			g_definition.name.c_str());
	}
}

void DrawMasterSection()
{
	if (!ImGui::CollapsingHeader("マスター", ImGuiTreeNodeFlags_DefaultOpen)) { return; }

	SliderFloatDirty("音量##master", &g_definition.masterVolume, 0.0f, 2.0f, "%.2f");
	CheckboxDirty("正規化", &g_definition.normalize);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("焼いたあとにピークを揃えます。レイヤーを足しても音量が暴れません");
	}
	if (g_definition.normalize) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		SliderFloatDirty("ピーク", &g_definition.normalizePeak, 0.1f, 1.0f, "%.2f");
	}

	ImGui::SetNextItemWidth(140.0f);
	const char* const rates[] = { "22050 Hz", "44100 Hz", "48000 Hz" };
	const int rateValues[] = { 22050, 44100, 48000 };
	int rateIndex = 1;
	for (int i = 0; i < 3; ++i) {
		if (g_definition.sampleRate == rateValues[i]) { rateIndex = i; }
	}
	if (ImGui::Combo("サンプルレート", &rateIndex, rates, 3)) {
		g_definition.sampleRate = rateValues[rateIndex];
		MarkDirty();
	}

	ImGui::SeparatorText("マスターフィルタ");
	EnumCombo("種類##masterfilter", &g_definition.masterFilterType,
		SoundDSP::FilterTypeNames(), SoundDSP::FilterTypeCount());
	if (g_definition.masterFilterType != SoundDSP::FilterType::None) {
		FrequencySlider("カットオフ##masterfilter", &g_definition.masterFilterCutoff);
		SliderFloatDirty("レゾナンス##masterfilter", &g_definition.masterFilterResonance, 0.1f, 8.0f, "%.2f");
	}
}

void DrawLayerBody(SELayer& layer)
{
	ImGui::SeparatorText("音源");
	EnumCombo("波形", &layer.wave, SoundDSP::WaveTypeNames(), SoundDSP::WaveTypeCount());

	const bool isNoise = (layer.wave == SoundDSP::WaveType::WhiteNoise || layer.wave == SoundDSP::WaveType::PinkNoise);

	if (!layer.pitchSweepEnabled) {
		FrequencySlider(isNoise ? "粒の細かさ" : "周波数", &layer.frequency);
		if (isNoise && ImGui::IsItemHovered()) {
			ImGui::SetTooltip("ノイズは指定した周波数で値を更新します。\n高いほど白色ノイズに近づき、低いほどザラつきます");
		}
	} else {
		ImGui::TextDisabled("周波数はピッチエンベロープが決めています");
	}

	if (layer.wave == SoundDSP::WaveType::Square) {
		SliderFloatDirty("デューティ比", &layer.pulseWidth, 0.05f, 0.95f, "%.2f");
	}
	if (isNoise) {
		if (ImGui::DragInt("乱数シード", &layer.noiseSeed, 1.0f, 1, 99999)) { MarkDirty(); }
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("同じ設定でも当たり外れが変わります。しっくり来ない時に回してください");
		}
	} else {
		SliderFloatDirty("FM 比率", &layer.fmRatio, 0.0f, 8.0f, "%.2f");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("整数から外すほど金属的になります（2.76 など）。0 で FM 無効");
		}
		SliderFloatDirty("FM 深さ", &layer.fmAmount, 0.0f, 3.0f, "%.2f");
	}

	ImGui::SeparatorText("時間・音量");
	SliderFloatDirty("長さ", &layer.duration, 0.01f, 3.0f, "%.3f 秒");
	SliderFloatDirty("開始", &layer.startDelay, 0.0f, 2.0f, "%.3f 秒");
	SliderFloatDirty("音量", &layer.volume, 0.0f, 1.5f, "%.2f");
	SliderFloatDirty("定位", &layer.pan, -1.0f, 1.0f, "%.2f");

	ImGui::SeparatorText("音量エンベロープ");
	SliderFloatDirty("Attack", &layer.envelope.attack, 0.0f, 1.0f, "%.3f 秒", ImGuiSliderFlags_Logarithmic);
	SliderFloatDirty("Hold", &layer.envelope.hold, 0.0f, 1.0f, "%.3f 秒", ImGuiSliderFlags_Logarithmic);
	SliderFloatDirty("Decay", &layer.envelope.decay, 0.0f, 2.0f, "%.3f 秒", ImGuiSliderFlags_Logarithmic);
	SliderFloatDirty("Sustain", &layer.envelope.sustain, 0.0f, 1.0f, "%.2f");
	SliderFloatDirty("Release", &layer.envelope.release, 0.0f, 1.0f, "%.3f 秒", ImGuiSliderFlags_Logarithmic);
	SliderFloatDirty("減衰カーブ", &layer.envelope.curve, 0.2f, 6.0f, "%.2f");
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("1 で直線。大きいほど最初に一気に落ちて、打撃音らしくなります");
	}
	if (layer.envelope.attack + layer.envelope.hold + layer.envelope.decay + layer.envelope.release > layer.duration) {
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "A+H+D+R が長さを超えたので、比率を保って縮めています");
	}

	ImGui::SeparatorText("ピッチエンベロープ");
	CheckboxDirty("ピッチを動かす", &layer.pitchSweepEnabled);
	if (layer.pitchSweepEnabled) {
		FrequencySlider("開始##pitch", &layer.pitchStart);
		FrequencySlider("終了##pitch", &layer.pitchEnd);
		SliderFloatDirty("掛ける時間", &layer.pitchSweepTime, 0.0f, 3.0f, "%.3f 秒");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("0 ならレイヤー全体を使って動かします");
		}
		EnumCombo("カーブ##pitch", &layer.pitchCurve, SoundDSP::SweepCurveNames(), SoundDSP::SweepCurveCount());
	}

	ImGui::SeparatorText("フィルタ");
	EnumCombo("種類##layerfilter", &layer.filterType, SoundDSP::FilterTypeNames(), SoundDSP::FilterTypeCount());
	if (layer.filterType != SoundDSP::FilterType::None) {
		FrequencySlider("カットオフ##layerfilter", &layer.filterCutoff);
		SliderFloatDirty("レゾナンス##layerfilter", &layer.filterResonance, 0.1f, 8.0f, "%.2f");
		CheckboxDirty("カットオフも動かす", &layer.filterSweepEnabled);
		if (layer.filterSweepEnabled) {
			FrequencySlider("終了##layerfilter", &layer.filterCutoffEnd);
		}
	}
}

void DrawLayerSection()
{
	if (!ImGui::CollapsingHeader("レイヤー", ImGuiTreeNodeFlags_DefaultOpen)) { return; }

	if (ImGui::Button("レイヤーを追加")) {
		SELayer layer;
		layer.name = "Layer " + std::to_string(g_definition.layers.size() + 1);
		g_definition.layers.push_back(layer);
		MarkDirty();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("%zu 本 / 合計 %.2f 秒", g_definition.layers.size(), g_definition.GetTotalDuration());

	// 並べ替えと削除は表を描き終えてから適用する。
	// 途中で vector を触ると参照が外れる
	int moveIndex = -1;
	int moveDirection = 0;
	int removeIndex = -1;
	int duplicateIndex = -1;

	for (size_t i = 0; i < g_definition.layers.size(); ++i) {
		SELayer& layer = g_definition.layers[i];
		ImGui::PushID(static_cast<int>(i));

		const std::string header = layer.name + " (" + SoundDSP::ToString(layer.wave) + ")";
		const bool open = ImGui::TreeNodeEx("##layer", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap,
			"%s", header.c_str());

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 200.0f);
		if (ImGui::Checkbox("##enabled", &layer.enabled)) { MarkDirty(); }
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("このレイヤーを鳴らす"); }
		ImGui::SameLine();
		if (ImGui::SmallButton("単体")) { PlayLayerSolo(layer); }
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("このレイヤーだけ鳴らす"); }
		ImGui::SameLine();
		if (ImGui::SmallButton("複製")) { duplicateIndex = static_cast<int>(i); }
		ImGui::SameLine();
		if (ImGui::SmallButton("↑")) { moveIndex = static_cast<int>(i); moveDirection = -1; }
		ImGui::SameLine();
		if (ImGui::SmallButton("↓")) { moveIndex = static_cast<int>(i); moveDirection = 1; }
		ImGui::SameLine();
		if (ImGui::SmallButton("×")) { removeIndex = static_cast<int>(i); }

		if (open) {
			char nameBuffer[64] = {};
			strncpy_s(nameBuffer, layer.name.c_str(), sizeof(nameBuffer) - 1);
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::InputText("レイヤー名", nameBuffer, sizeof(nameBuffer))) {
				layer.name = nameBuffer;
				g_unsaved = true;
			}
			DrawLayerBody(layer);
			ImGui::TreePop();
		}

		ImGui::PopID();
		ImGui::Separator();
	}

	if (duplicateIndex >= 0) {
		SELayer copy = g_definition.layers[duplicateIndex];
		copy.name += " copy";
		g_definition.layers.insert(g_definition.layers.begin() + duplicateIndex + 1, copy);
		MarkDirty();
	}
	if (moveIndex >= 0) {
		const int target = moveIndex + moveDirection;
		if (target >= 0 && target < static_cast<int>(g_definition.layers.size())) {
			std::swap(g_definition.layers[moveIndex], g_definition.layers[target]);
			MarkDirty();
		}
	}
	if (removeIndex >= 0) {
		g_definition.layers.erase(g_definition.layers.begin() + removeIndex);
		MarkDirty();
	}
}

void DrawEffectSection()
{
	if (!ImGui::CollapsingHeader("エフェクト")) { return; }

	ImGui::SeparatorText("歪み");
	CheckboxDirty("有効##distortion", &g_definition.distortion.enabled);
	if (g_definition.distortion.enabled) {
		SliderFloatDirty("ドライブ", &g_definition.distortion.drive, 0.1f, 20.0f, "%.2f");
		SliderFloatDirty("かかり具合##distortion", &g_definition.distortion.mix, 0.0f, 1.0f, "%.2f");
	}

	ImGui::SeparatorText("ディレイ");
	CheckboxDirty("有効##delay", &g_definition.delay.enabled);
	if (g_definition.delay.enabled) {
		SliderFloatDirty("間隔", &g_definition.delay.time, 0.01f, 1.0f, "%.3f 秒");
		SliderFloatDirty("フィードバック", &g_definition.delay.feedback, 0.0f, 0.95f, "%.2f");
		SliderFloatDirty("かかり具合##delay", &g_definition.delay.mix, 0.0f, 1.0f, "%.2f");
	}

	ImGui::SeparatorText("リバーブ");
	CheckboxDirty("有効##reverb", &g_definition.reverb.enabled);
	if (g_definition.reverb.enabled) {
		SliderFloatDirty("広さ", &g_definition.reverb.roomSize, 0.0f, 1.0f, "%.2f");
		SliderFloatDirty("吸収", &g_definition.reverb.damping, 0.0f, 1.0f, "%.2f");
		SliderFloatDirty("ステレオ幅", &g_definition.reverb.width, 0.0f, 1.0f, "%.2f");
		SliderFloatDirty("かかり具合##reverb", &g_definition.reverb.mix, 0.0f, 1.0f, "%.2f");
	}
}

} // namespace

void Editor::DrawSoundEditorWindow()
{
	// 一覧と編集を横に並べるので、初期サイズは広めにしておく。
	// FirstUseEver なので、ユーザーが動かした後は保存された配置が優先される
	ImGui::SetNextWindowSize(ImVec2(760.0f, 620.0f), ImGuiCond_FirstUseEver);

	if (!EditorWindow::Begin("Sound Editor", EditorWindow::Category::kEngine, 0, false)) {
		return;
	}

	if (!Ctx().audio) {
		ImGui::TextDisabled("EditorContext がまだ設定されていません");
		EditorWindow::End();
		return;
	}

	// 左：ファイル一覧
	if (ImGui::BeginChild("##list", ImVec2(210.0f, 0.0f), ImGuiChildFlags_Borders)) {
		DrawFileList();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// 右：編集
	if (ImGui::BeginChild("##editor", ImVec2(0.0f, 0.0f))) {
		DrawTransport();

		ImGui::Separator();
		DrawWaveform(110.0f);
		DrawSpectrum(80.0f);
		ImGui::TextDisabled("長さ %.3f 秒 / ピーク %.2f / 生成 %.1f ms",
			g_preview.DurationSeconds(), g_preview.PeakLevel(), g_lastRenderMs);
		ImGui::Separator();

		DrawMasterSection();
		DrawLayerSection();
		DrawEffectSection();
	}
	ImGui::EndChild();

	// 焼き直しは「どのスライダーも掴んでいない」タイミングまで待つ。
	// ドラッグ中に毎フレーム焼くと、長い音ではフレームレートが落ちる
	if (g_needsRender && !ImGui::IsAnyItemActive()) {
		RenderPreview();
		if (g_autoPlay) { PlayPreview(); }
	}

	EditorWindow::End();
}

#endif // _DEBUG
