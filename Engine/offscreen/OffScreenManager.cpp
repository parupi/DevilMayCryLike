#include "OffScreenManager.h"
#include "base/SrvManager.h"

OffScreenManager* OffScreenManager::instance = nullptr;
std::once_flag OffScreenManager::initInstanceFlag;

OffScreenManager* OffScreenManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new OffScreenManager();
		});
	return instance;
}

void OffScreenManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager, SrvManager* srvManager)
{
	dxManager_ = dxManager;
	psoManager_ = psoManager;
	srvManager_ = srvManager;

	// CreateTexture2DResource 相当の関数を使って２つ生成
	pingPongBuffers_[ping_] = CreateOffScreenRenderTarget();
	pingPongBuffers_[pong_] = CreateOffScreenRenderTarget();

	rtvHeap_ = dxManager_->CreateDescriptorHeap(
		dxManager_->GetDevice(),
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		2,         // ピンポン用2つ分
		false      // shader visible = false
	);

	descriptorSizeRTV_ = dxManager_->GetDescriptorSizeRTV();

	D3D12_CPU_DESCRIPTOR_HANDLE baseHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	for (uint32_t i = 0; i < 2; ++i) {
		rtvHandles_[i].ptr = baseHandle.ptr + i * descriptorSizeRTV_;
	}

	// RTV, SRV をそれぞれ作成
	for (uint32_t i = 0; i < 2; ++i) {
		rtvHandles_[i] = CreateRTV(pingPongBuffers_[i]);
		srvHandles_[i] = CreateSRV(pingPongBuffers_[i]);
	}
}

void OffScreenManager::Finalize()
{
	// オフスクリーンエフェクトの解放
	effects_.clear();

	dxManager_ = nullptr;
	psoManager_ = nullptr;

	delete instance;
	instance = nullptr;
}

void OffScreenManager::Update()
{
	for (size_t i = 0; i < effects_.size(); i++) {
		effects_[i]->Update();
	}
}

void OffScreenManager::DrawPostEffect()
{
	bool isDraw = false;

	// 今回使うSRVは直前に描画したPingバッファ（pongがそのSRVになる）
	D3D12_GPU_DESCRIPTOR_HANDLE inputSrv = srvHandles_[pong_];

	for (auto& effect : effects_) {
		if (!effect->IsActive()) continue;

		//// 書き込み先（pingを描画先に）
		//dxManager_->GetCommandContext()->TransitionResource(
		//	pingPongBuffers_[ping_].Get(),
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	D3D12_RESOURCE_STATE_RENDER_TARGET
		//);


		//dxManager_->SetRenderTarget(rtvHandles_[ping_]);
		//dxManager_->GetCommandContext()->ClearRenderTarget(rtvHandles_[ping_], clearValue_.Color);

		// SRVセット
		effect->SetInputTexture(srvHandles_[ping_]);
		effect->Draw();

		//// 書き終わったPingを読み込み状態に戻す
		//dxManager_->GetCommandContext()->TransitionResource(
		//	pingPongBuffers_[ping_].Get(),
		//	D3D12_RESOURCE_STATE_RENDER_TARGET,
		//	D3D12_RESOURCE_STATE_GENERIC_READ
		//);

		//std::swap(ping_, pong_);

		isDraw = true;
	}

	if (isDraw) {
		//// 書き終わったPingを読み込み状態に戻す
		//dxManager_->GetCommandContext()->TransitionResource(
		//	pingPongBuffers_[ping_].Get(),
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	D3D12_RESOURCE_STATE_RENDER_TARGET
		//);
	}

	// もし描画されていなければ
	if (!isDraw) {
		dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kNone));
		dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
		dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(0, srvHandles_[ping_]);
		dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxManager_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
		Logger::Log("complet draw\n");
	}
}

void OffScreenManager::AddEffect(std::unique_ptr<BaseOffScreen> effect)
{
	effects_.emplace_back(std::move(effect));
}

BaseOffScreen* OffScreenManager::FindEffect(const std::string& name)
{
	for (auto& effect : effects_) {
		if (!effect) continue;

		if (effect->GetName() == name) {
			return effect.get();
		}
	}
}

void OffScreenManager::BeginDrawToPingPong()
{
	dxManager_->GetCommandContext()->TransitionResource(
		pingPongBuffers_[ping_].Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// 2. レンダーターゲット設定
	dxManager_->SetRenderTarget(rtvHandles_[ping_]);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxManager_->GetDSVHandle();

	dxManager_->GetCommandContext()->SetRenderTarget(rtvHandles_[ping_], dsvHandle);
	// 3. クリア
	dxManager_->GetCommandContext()->ClearRenderTarget(rtvHandles_[ping_], clearValue_.Color);

	// 4. ビューポートとシザーの設定
	dxManager_->GetCommandContext()->SetViewportAndScissor(viewport_, scissorRect_);
}

void OffScreenManager::EndDrawToPingPong()
{
	// Pingバッファの描画完了 → 読み込み可能状態に遷移

	dxManager_->GetCommandContext()->TransitionResource(
		pingPongBuffers_[ping_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);

	// PingとPongを入れ替える
	std::swap(ping_, pong_);
}

Microsoft::WRL::ComPtr<ID3D12Resource> OffScreenManager::CreateOffScreenRenderTarget()
{
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport_.Width = WindowManager::kClientWidth;
	viewport_.Height = WindowManager::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect_.left = 0;
	scissorRect_.right = WindowManager::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WindowManager::kClientHeight;

	clearValue_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearValue_.Color[0] = 0.6f;
	clearValue_.Color[1] = 0.5f;
	clearValue_.Color[2] = 0.1f;
	clearValue_.Color[3] = 1.0f;
	
	return dxManager_->CreateRenderTextureResource(WindowManager::kClientWidth, WindowManager::kClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
}

D3D12_CPU_DESCRIPTOR_HANDLE OffScreenManager::CreateRTV(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
	// 静的にインデックスを持つ（実際は複数回呼ぶ用）
	static uint32_t rtvCurrentIndex = 0;

	assert(rtvCurrentIndex < _countof(rtvHandles_)); // 最大数を超えないように

	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHandles_[rtvCurrentIndex++];

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	dxManager_->GetDevice()->CreateRenderTargetView(resource.Get(), &rtvDesc, handle);

	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE OffScreenManager::CreateSRV(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
	int32_t srvIndex = srvManager_->Allocate();

	D3D12_GPU_DESCRIPTOR_HANDLE handle = srvManager_->GetGPUDescriptorHandle(srvIndex);

	srvManager_->CreateSRVforTexture2D(srvIndex, resource.Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1);

	return handle;
}
