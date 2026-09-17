#include "AssetBrowserWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorAssetUtil.h"
#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"
#include "Editor/Windows/HierarchyWindow.h"

#include "Debugger/ImGuiManager.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/TextureManager.h"
#include "World3D/Object/Model/BaseModel.h"
#include "World3D/Object/Model/Material/Material.h"
#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Object/Object3d.h"   // 「配置」で作った Object3d の name_ を読む

#include <algorithm>
#include <imgui/imgui.h>

namespace {

std::vector<std::string> g_diskModels;
bool g_diskModelsScanned = false;

std::vector<std::string> g_textureNames;
size_t g_textureCountWhenScanned = static_cast<size_t>(-1);

char g_filter[64] = "";

// --- テクスチャプレビュー ---
// TextureManager が持つSRVはエンジン側のシェーダ可視ヒープにあるので、ImGui::Image には渡せない
// （ImGuiは自分のヒープしか SetDescriptorHeaps しない）。
// シェーダ可視ヒープ同士は CopyDescriptors できないので、ImGuiのヒープに「1枠だけ」確保して、
// 選択が変わるたびにリソースからSRVを作り直す。枠を1つに固定しておけば
// ImGuiのヒープ(64枠)を食い潰す心配もない
std::string g_previewName;
bool g_previewSlotReady = false;
D3D12_CPU_DESCRIPTOR_HANDLE g_previewCpu{};
D3D12_GPU_DESCRIPTOR_HANDLE g_previewGpu{};

bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle)
{
	if (needle.empty()) {
		return true;
	}
	auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
		[](char a, char b) {
			return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		});
	return it != haystack.end();
}

// プレビュー対象を切り替える。同じものなら何もしない
void SetPreviewTexture(const std::string& name)
{
	if (g_previewName == name) {
		return;
	}
	TextureManager* textures = Editor::Ctx().textureManager;
	DirectXManager* dx = Editor::Ctx().dxManager;
	if (!textures || !dx) {
		return;
	}
	ID3D12Resource* resource = textures->GetResource(name);
	if (!resource) {
		return;
	}

	if (!g_previewSlotReady) {
		if (!ImGuiManager::GetInstance().AllocateSrvDescriptor(&g_previewCpu, &g_previewGpu)) {
			return;
		}
		g_previewSlotReady = true;
	}

	const DirectX::TexMetadata* meta = textures->TryGetMetaData(name);
	if (!meta) {
		return;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
	desc.Format = meta->format;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Texture2D.MipLevels = static_cast<UINT>(meta->mipLevels);
	dx->GetDevice()->CreateShaderResourceView(resource, &desc, g_previewCpu);

	g_previewName = name;
}

const char* FormatName(DXGI_FORMAT format)
{
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:      return "RGBA8";
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "RGBA8 sRGB";
	case DXGI_FORMAT_BC1_UNORM:           return "BC1";
	case DXGI_FORMAT_BC1_UNORM_SRGB:      return "BC1 sRGB";
	case DXGI_FORMAT_BC3_UNORM:           return "BC3";
	case DXGI_FORMAT_BC3_UNORM_SRGB:      return "BC3 sRGB";
	case DXGI_FORMAT_BC7_UNORM:           return "BC7";
	case DXGI_FORMAT_BC7_UNORM_SRGB:      return "BC7 sRGB";
	default:                              return "その他";
	}
}

// --- モデルタブ ---

void DrawModelTab()
{
	ModelManager* models = Editor::Ctx().modelManager;
	if (!models) {
		ImGui::TextDisabled("ModelManager がありません");
		return;
	}

	if (!g_diskModelsScanned) {
		g_diskModels = Editor::ScanModelFolders();
		g_diskModelsScanned = true;
	}

	if (ImGui::Button("再スキャン")) {
		g_diskModelsScanned = false;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##modelFilter", "名前で絞り込み", g_filter, sizeof(g_filter));

	const std::string needle = g_filter;

	ImGui::Separator();
	ImGui::TextDisabled("Resource/Models にある %zu 件 / 読み込み済み %zu 件",
		g_diskModels.size(), models->models.size() + models->skinnedModels.size());

	if (!ImGui::BeginTable("##models", 4,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
		return;
	}
	ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("状態", ImGuiTableColumnFlags_WidthFixed, 90.0f);
	ImGui::TableSetupColumn("マテリアル", ImGuiTableColumnFlags_WidthFixed, 80.0f);
	ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 150.0f);
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableHeadersRow();

	for (const std::string& name : g_diskModels) {
		if (!ContainsIgnoreCase(name, needle)) {
			continue;
		}
		ImGui::PushID(name.c_str());
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(name.c_str());

		BaseModel* loaded = models->FindModel(name);

		ImGui::TableNextColumn();
		if (loaded) {
			ImGui::TextUnformatted("読み込み済み");
		} else {
			ImGui::TextDisabled("未読み込み");
		}

		ImGui::TableNextColumn();
		if (loaded) {
			ImGui::Text("%zu", loaded->GetMaterials().size());
		} else {
			ImGui::TextDisabled("-");
		}

		ImGui::TableNextColumn();
		if (!loaded) {
			if (ImGui::SmallButton("読み込む")) {
				models->LoadModel(name);
				EditorMenuBar::ShowToast("%s を読み込みました", name.c_str());
			}
		} else if (ImGui::SmallButton("配置")) {
			// Hierarchy の生成と同じ経路を通す
			if (Object3d* created = Editor::CreateModelObject(name, name)) {
				EditorMenuBar::ShowToast("%s を配置しました", created->name_.c_str());
			}
		}

		ImGui::PopID();
	}

	ImGui::EndTable();
}

// --- テクスチャタブ ---

void DrawTextureTab()
{
	TextureManager* textures = Editor::Ctx().textureManager;
	if (!textures) {
		ImGui::TextDisabled("TextureManager がありません");
		return;
	}

	// 読み込みは実行中に増えるので、枚数が変わったときだけ取り直す
	if (g_textureCountWhenScanned != textures->GetLoadedTextureCount()) {
		g_textureNames = textures->GetLoadedTextureNames();
		g_textureCountWhenScanned = textures->GetLoadedTextureCount();
	}

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##texFilter", "名前で絞り込み", g_filter, sizeof(g_filter));
	const std::string needle = g_filter;

	ImGui::TextDisabled("読み込み済み %zu 枚", g_textureNames.size());
	ImGui::Separator();

	// 左に一覧、右にプレビュー
	const float previewWidth = 260.0f;
	if (ImGui::BeginChild("##texList", ImVec2(-previewWidth, 0.0f))) {
		if (ImGui::BeginTable("##textures", 4,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
			ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("サイズ", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("形式", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Mip", ImGuiTableColumnFlags_WidthFixed, 40.0f);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			for (const std::string& name : g_textureNames) {
				if (!ContainsIgnoreCase(name, needle)) {
					continue;
				}
				const DirectX::TexMetadata* meta = textures->TryGetMetaData(name);
				if (!meta) {
					continue;
				}

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (ImGui::Selectable(name.c_str(), g_previewName == name, ImGuiSelectableFlags_SpanAllColumns)) {
					SetPreviewTexture(name);
				}
				ImGui::TableNextColumn();
				ImGui::Text("%zux%zu", meta->width, meta->height);
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(FormatName(meta->format));
				ImGui::TableNextColumn();
				ImGui::Text("%zu", meta->mipLevels);
			}
			ImGui::EndTable();
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	if (ImGui::BeginChild("##texPreview", ImVec2(0.0f, 0.0f))) {
		const DirectX::TexMetadata* meta =
			g_previewName.empty() ? nullptr : textures->TryGetMetaData(g_previewName);
		if (!meta) {
			ImGui::TextDisabled("テクスチャを選ぶと\nここに表示されます");
		} else {
			ImGui::TextWrapped("%s", g_previewName.c_str());

			// 縦横比を保ったまま枠に収める
			const float avail = (std::max)(64.0f, ImGui::GetContentRegionAvail().x);
			const float aspect = (meta->height > 0)
				? static_cast<float>(meta->width) / static_cast<float>(meta->height) : 1.0f;
			const float width = (std::min)(avail, 240.0f);
			// 透明なテクスチャでも輪郭が分かるよう、背景に市松模様代わりの枠を敷く
			const ImVec2 topLeft = ImGui::GetCursorScreenPos();
			const ImVec2 size(width, width / aspect);
			ImGui::GetWindowDrawList()->AddRectFilled(
				topLeft, ImVec2(topLeft.x + size.x, topLeft.y + size.y), IM_COL32(70, 70, 70, 255));
			ImGui::Image(static_cast<ImTextureID>(g_previewGpu.ptr), size);
			ImGui::GetWindowDrawList()->AddRect(
				topLeft, ImVec2(topLeft.x + size.x, topLeft.y + size.y), IM_COL32(120, 120, 120, 255));

			ImGui::TextDisabled("%zux%zu  %s  mip%zu",
				meta->width, meta->height, FormatName(meta->format), meta->mipLevels);
		}
	}
	ImGui::EndChild();
}

} // namespace

void Editor::DrawAssetBrowserWindow()
{
	if (!EditorWindow::Begin("Asset Browser", EditorWindow::Category::kEngine)) {
		return;
	}

	if (ImGui::BeginTabBar("##assets")) {
		if (ImGui::BeginTabItem("モデル")) {
			DrawModelTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("テクスチャ")) {
			DrawTextureTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	EditorWindow::End();
}

#endif // _DEBUG
