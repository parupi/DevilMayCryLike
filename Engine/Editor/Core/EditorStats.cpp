#include "EditorStats.h"
#ifdef _DEBUG

#include "EditorWindowRegistry.h"
#include "Utility/DeltaTime.h"
#include <imgui/imgui.h>

#include <Windows.h>
#include <dxgi1_6.h>
#include <psapi.h>
#include <wrl/client.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "psapi.lib")

namespace {

constexpr int kHistoryCount = 180; // 3秒ぶん(60FPS想定)

float g_frameTimeMs[kHistoryCount] = {};
int g_historyOffset = 0;

// 重い問い合わせなので毎フレームは引かない
float g_pollTimer = 0.0f;
constexpr float kPollInterval = 0.5f;

float g_vramUsedMB = 0.0f;
float g_vramBudgetMB = 0.0f;
float g_ramUsedMB = 0.0f;

Microsoft::WRL::ComPtr<IDXGIAdapter3> g_adapter;

// VRAM量を引くためだけにアダプタを掴む。
// GraphicsDevice はアダプタを保持していないので、ここで自前に取り直している
IDXGIAdapter3* GetAdapter()
{
	if (g_adapter) {
		return g_adapter.Get();
	}

	Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
		return nullptr;
	}
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
	if (FAILED(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter1)))) {
		return nullptr;
	}
	if (FAILED(adapter1.As(&g_adapter))) {
		return nullptr;
	}
	return g_adapter.Get();
}

void PollMemory()
{
	constexpr float kToMB = 1.0f / (1024.0f * 1024.0f);

	if (IDXGIAdapter3* adapter = GetAdapter()) {
		DXGI_QUERY_VIDEO_MEMORY_INFO info{};
		if (SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
			g_vramUsedMB = static_cast<float>(info.CurrentUsage) * kToMB;
			g_vramBudgetMB = static_cast<float>(info.Budget) * kToMB;
		}
	}

	PROCESS_MEMORY_COUNTERS_EX counters{};
	counters.cb = sizeof(counters);
	if (GetProcessMemoryInfo(GetCurrentProcess(),
			reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
		g_ramUsedMB = static_cast<float>(counters.WorkingSetSize) * kToMB;
	}
}

} // namespace

void EditorStats::Update()
{
	// ポーズ中も動き続ける実測値を使う
	const float delta = DeltaTime::GetUnscaledDeltaTime();

	g_frameTimeMs[g_historyOffset] = delta * 1000.0f;
	g_historyOffset = (g_historyOffset + 1) % kHistoryCount;

	g_pollTimer -= delta;
	if (g_pollTimer <= 0.0f) {
		g_pollTimer = kPollInterval;
		PollMemory();
	}
}

void EditorStats::DrawMenuBarSummary()
{
	const ImGuiIO& io = ImGui::GetIO();

	char buffer[160];
	snprintf(buffer, sizeof(buffer), "%.0f FPS  |  %.2f ms  |  VRAM %.0f MB  |  RAM %.0f MB",
		io.Framerate, 1000.0f / (io.Framerate > 0.0f ? io.Framerate : 1.0f), g_vramUsedMB, g_ramUsedMB);

	// メニューバーの右端に寄せる
	const float textWidth = ImGui::CalcTextSize(buffer).x;
	const float available = ImGui::GetContentRegionAvail().x;
	if (available > textWidth) {
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + available - textWidth - ImGui::GetStyle().ItemSpacing.x);
	}

	// 予算を超えたら赤くする。VRAMは超えた瞬間に一気に重くなるので目立たせたい
	const bool overBudget = (g_vramBudgetMB > 0.0f) && (g_vramUsedMB > g_vramBudgetMB);
	if (overBudget) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
	}
	ImGui::TextUnformatted(buffer);
	if (overBudget) {
		ImGui::PopStyleColor();
	}

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("VRAM %.0f / %.0f MB（予算）\nRAM (WorkingSet) %.0f MB",
			g_vramUsedMB, g_vramBudgetMB, g_ramUsedMB);
	}
}

void EditorStats::DrawWindow()
{
	if (!EditorWindow::Begin("Stats", EditorWindow::Category::kEngine, 0, false)) {
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("FPS        : %.1f", io.Framerate);
	ImGui::Text("フレーム時間 : %.3f ms", 1000.0f / (io.Framerate > 0.0f ? io.Framerate : 1.0f));

	// 直近のスパイクが見えるように、履歴の最大値でスケールを決める
	float maxMs = 16.6f;
	for (float ms : g_frameTimeMs) {
		maxMs = (ms > maxMs) ? ms : maxMs;
	}
	ImGui::PlotLines("##frametime", g_frameTimeMs, kHistoryCount, g_historyOffset,
		"フレーム時間 (ms)", 0.0f, maxMs, ImVec2(0.0f, 80.0f));

	ImGui::Separator();
	ImGui::Text("VRAM 使用量 : %.1f MB", g_vramUsedMB);
	ImGui::Text("VRAM 予算   : %.1f MB", g_vramBudgetMB);
	if (g_vramBudgetMB > 0.0f) {
		ImGui::ProgressBar(g_vramUsedMB / g_vramBudgetMB, ImVec2(-1.0f, 0.0f));
	}
	ImGui::Text("RAM (WorkingSet) : %.1f MB", g_ramUsedMB);

	ImGui::Separator();
	ImGui::Text("登録済みエディタウィンドウ : %zu", EditorWindow::GetAllWindowNames().size());

	EditorWindow::End();
}

void EditorStats::Finalize()
{
	g_adapter.Reset();
}

#endif // _DEBUG
