#include "EditorGameView.h"
#ifdef _DEBUG

#include "Graphics/Device/DirectXManager.h"
#include "Editor/Core/EditorWindowRegistry.h"

namespace {
// バックバッファと同じフォーマットにしておく。
// 最終合成PSO・スプライトPSOがこのフォーマット前提で作られているため、
// ここを変えるとPSOのRTVFormatと食い違ってしまう。
constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
} // namespace

void EditorGameView::Initialize(DirectXManager* dxManager,
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpu, D3D12_GPU_DESCRIPTOR_HANDLE srvGpu)
{
	dxManager_ = dxManager;
	srvCpu_ = srvCpu;
	srvGpu_ = srvGpu;

	GpuResourceFactory::TextureDesc desc{};
	desc.width = kWidth;
	desc.height = kHeight;
	desc.format = kFormat;
	desc.usage = GpuResourceFactory::Usage::RenderTarget;
	desc.clearColor[0] = 0.0f;
	desc.clearColor[1] = 0.0f;
	desc.clearColor[2] = 0.0f;
	desc.clearColor[3] = 1.0f;

	renderTarget_ = dxManager_->GetResourceFactory()->CreateTexture2D(desc);
	renderTarget_->SetName(L"EditorGameView");

	rtvIndex_ = dxManager_->GetRtvManager()->Allocate();
	dxManager_->GetRtvManager()->CreateRTV(rtvIndex_, renderTarget_.Get());

	// SRVはImGuiのディスクリプタヒープ側に作る。
	// ImGui::Image に渡すGPUハンドルは、ImGuiが描画時にSetDescriptorHeapsする
	// ヒープの中を指していないといけない
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = kFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	// RGBはそのまま、Aだけ常に1にする。
	// 最終合成やUIスプライトが書き残したアルファをそのまま読むと、ImGui::Image が
	// 半透明合成になって後ろのウィンドウが透けてしまうため
	srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
		0, 1, 2, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
	srvDesc.Texture2D.MipLevels = 1;
	dxManager_->GetDevice()->CreateShaderResourceView(renderTarget_.Get(), &srvDesc, srvCpu_);

	viewport_.Width = static_cast<float>(kWidth);
	viewport_.Height = static_cast<float>(kHeight);
	viewport_.TopLeftX = 0.0f;
	viewport_.TopLeftY = 0.0f;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	scissorRect_.left = 0;
	scissorRect_.top = 0;
	scissorRect_.right = static_cast<LONG>(kWidth);
	scissorRect_.bottom = static_cast<LONG>(kHeight);

	// CreateTexture2D はRENDER_TARGETステートで作る
	isRenderTargetState_ = true;
}

void EditorGameView::Finalize()
{
	renderTarget_.Reset();
	dxManager_ = nullptr;
}

void EditorGameView::Begin()
{
	auto* commandCtx = dxManager_->GetCommandContext();

	if (!isRenderTargetState_) {
		commandCtx->TransitionResource(renderTarget_.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		isRenderTargetState_ = true;
	}

	auto rtv = dxManager_->GetRtvManager()->GetCPUDescriptorHandle(rtvIndex_);
	commandCtx->SetRenderTarget(rtv);

	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandCtx->ClearRenderTarget(rtv, clearColor);
	commandCtx->SetViewportAndScissor(viewport_, scissorRect_);
}

void EditorGameView::End()
{
	if (!isRenderTargetState_) return;

	dxManager_->GetCommandContext()->TransitionResource(renderTarget_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	isRenderTargetState_ = false;
}

void EditorGameView::DrawWindow()
{
	const ImVec2 imageSize(static_cast<float>(kWidth), static_cast<float>(kHeight));

	// 初回はコンテンツ領域がちょうど1280x720になる大きさで開く
	// （タイトルバーのぶんだけ縦に足す）
	ImGui::SetNextWindowSize(
		ImVec2(imageSize.x, imageSize.y + ImGui::GetFrameHeight()), ImGuiCond_FirstUseEver);

	// 画像をウィンドウいっぱいに敷きたいので余白を消す。
	// ウィンドウを1280x720より小さくされたときはスクロールで全体を追えるようにする
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	const bool opened = EditorWindow::Begin("Game", EditorWindow::Category::kGame, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::PopStyleVar();

	if (opened) {
		hovered_ = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
		focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

		// 拡縮せず常に1280x720の等倍で描く。
		// 領域が余っているときだけ中央に寄せる
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(
			cursor.x + (std::max)(0.0f, (avail.x - imageSize.x) * 0.5f),
			cursor.y + (std::max)(0.0f, (avail.y - imageSize.y) * 0.5f)));

		ImGui::Image(GetTextureID(), imageSize);
		EditorWindow::End();
	} else {
		// 非表示にされたか折りたたまれている。どちらもEnd()は不要
		hovered_ = false;
		focused_ = false;
	}
}

#endif // _DEBUG
