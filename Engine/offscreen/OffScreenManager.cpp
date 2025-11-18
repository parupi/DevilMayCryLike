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

void OffScreenManager::Initialize(DirectXManager* dxManager, PSOManager* psoManager)
{
	assert(dxManager);
	dxManager_ = dxManager;
	psoManager_ = psoManager;
	//srvManager_ = dxManager_->GetSrvManager();

	viewport_ = { 0.0f, 0.0f, WindowManager::kClientWidth, WindowManager::kClientHeight, 0.0f, 1.0f };
	scissorRect_ = { 0, 0, WindowManager::kClientWidth, WindowManager::kClientHeight };

	clearValue_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearValue_.Color[0] = powf(0.6f, 2.2f);
	clearValue_.Color[1] = powf(0.5f, 2.2f);
	clearValue_.Color[2] = powf(0.1f, 2.2f);
	clearValue_.Color[3] = 1.0f;

	// ping/pong を作る（リソース作成 → RTV/SRV を Manager を通して作成）
	for (int i = 0; i < 2; ++i) {
		pingPongBuffers_[i] = CreateOffScreenRenderTarget();
		// create RTV index via RtvManager and create SRV index via SrvManager
		rtvIndices_[i] = CreateRTVForResource(pingPongBuffers_[i]);
		srvIndices_[i] = CreateSRVForResource(pingPongBuffers_[i]);

		// store handles for convenience
		rtvHandles_[i] = dxManager_->GetRtvManager()->GetCPUDescriptorHandle(rtvIndices_[i]);
		srvHandles_[i] = dxManager_->GetSrvManager()->GetGPUDescriptorHandle(srvIndices_[i]);
	}

	ping_ = 0;
	pong_ = 1;
}

void OffScreenManager::Finalize()
{
	// オフスクリーンエフェクトの解放
	effects_.clear();

	for (auto& buffer : pingPongBuffers_) {
		buffer.Reset();
	}

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
	//bool didDraw = false;

	//// 現在描画先は ping_, 入力は pong_
	//// input SRV は pong_（直前に描画されたバッファ）
	//D3D12_GPU_DESCRIPTOR_HANDLE inputSrv = srvHandles_[pong_];

	//for (auto& effect : effects_) {
	//	if (!effect || !effect->IsActive()) continue;

	//	// 1) Set SRV for effect (effect->Draw()は現在のコマンドリストに描画コマンドを出す想定)
	//	effect->SetInputTexture(inputSrv);

	//	// 2) Make ping_ writable (GENERIC_READ -> RENDER_TARGET), set RTV and clear
	//	dxManager_->GetCommandContext()->TransitionResource(
	//		pingPongBuffers_[ping_].Get(),
	//		D3D12_RESOURCE_STATE_GENERIC_READ,
	//		D3D12_RESOURCE_STATE_RENDER_TARGET
	//	);

	//	// set render target + dsv
	//	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxManager_->GetDsvManager()->GetDsvHandle();
	//	dxManager_->GetCommandContext()->SetRenderTarget(rtvHandles_[ping_], dsvHandle);

	//	// clear
	//	dxManager_->GetCommandContext()->ClearRenderTarget(rtvHandles_[ping_], clearValue_.Color);
	//	dxManager_->GetCommandContext()->SetViewportAndScissor(viewport_, scissorRect_);

	//	// 3) Draw effect (effect->Draw は root signature / PSO / SRV を適切に設定するものとする)
	//	effect->Draw();

	//	// 4) finished -> make ping_ readable again
	//	dxManager_->GetCommandContext()->TransitionResource(
	//		pingPongBuffers_[ping_].Get(),
	//		D3D12_RESOURCE_STATE_RENDER_TARGET,
	//		D3D12_RESOURCE_STATE_GENERIC_READ
	//	);

	//	// swap roles for next effect
	//	std::swap(ping_, pong_);

	//	// update inputSrv for next effect (now the most recent rendered result is at pong_)
	//	inputSrv = srvHandles_[pong_];

	//	didDraw = true;
	//}

	//// if nothing drawn, draw a simple full-screen triangle that copies ping_ (or keep previous content)
	//if (!didDraw) {
	//	// set PSO + root signature for a fallback draw
	//	auto cmdList = dxManager_->GetCommandList();
	//	cmdList->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kNone));
	//	cmdList->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	//	cmdList->SetGraphicsRootDescriptorTable(0, srvHandles_[ping_]); // show current ping
	//	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//	cmdList->DrawInstanced(3, 1, 0, 0);
	//}

	//// 最終結果は pong_ 側
	//auto finalSrv = srvHandles_[pong_];

	////// バックバッファを RenderTarget として再セットする
	////UINT backBufferIndex = dxManager_->GetSwapChainManager()->GetCurrentBackBufferIndex();
	////auto backRtv = dxManager_->GetRtvManager()->GetCPUDescriptorHandle(backBufferIndex);
	////auto backDsv = dxManager_->GetDsvManager()->GetDsvHandle();

	////// BackBufferは BeginDraw() でRT状態になっている想定なのでBarrier不要
	////dxManager_->GetCommandContext()->SetRenderTarget(backRtv, backDsv);

	////// fullscreen triangleを1回だけ描画してfinalSrvをBackBufferへ表示
	////auto cmdList = dxManager_->GetCommandList();
	////cmdList->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kNone));
	////cmdList->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	////cmdList->SetGraphicsRootDescriptorTable(0, finalSrv);
	////cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	////cmdList->DrawInstanced(3, 1, 0, 0);

	//// 最終SRVをメンバーに保存しておく（Lightingで使えるように）
	//finalPostEffectSrv_ = srvHandles_[pong_];

	//// (任意) ここでフラグや状態をセットしておく
	//didHavePostEffectResult_ = didDraw;
}

void OffScreenManager::ExecutePostEffects()
{
	auto* context = dxManager_->GetCommandContext();

	D3D12_GPU_DESCRIPTOR_HANDLE inputSrv = srvHandles_[pong_];
	// 1つでも有効なpathが動いたか判定
	bool anyExecuted = false;

	for (auto& path : paths_) {
		if (!path->IsActive()) continue;
		// 実行された
		anyExecuted = true;
		path->SetInputSRV(inputSrv);
		path->SetOutput(pingPongBuffers_[ping_].Get(), rtvHandles_[ping_]);
		path->SetViewport(viewport_, scissorRect_);

		path->Execute();

		context->TransitionResource(
			pingPongBuffers_[ping_].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);

		std::swap(ping_, pong_);
		inputSrv = srvHandles_[pong_];

		finalPostEffectSrv_ = path->GetOutputSRV();
	}
	// pathが一つも無かった場合のフォールバック
	if (!anyExecuted) {
		// 最初の入力を最終出力にする
		finalPostEffectSrv_ = srvHandles_[pong_];
	}
}

void OffScreenManager::AddEffect(std::unique_ptr<BaseOffScreen> effect)
{
	PostEffectPath* path = new PostEffectPath(effect.get());
	paths_.emplace_back(path);
	effects_.push_back(std::move(effect));
}

BaseOffScreen* OffScreenManager::FindEffect(const std::string& name)
{
	for (auto& effect : effects_) {
		if (!effect) continue;

		if (effect->GetName() == name) {
			return effect.get();
		}
	}
	return nullptr;
}

std::vector<BaseOffScreen*> OffScreenManager::GetEffects()
{
	std::vector<BaseOffScreen*> effects;
	for (auto& effect : effects_) {
		effects.push_back(effect.get());
	}
	return effects;
}

void OffScreenManager::BeginDrawToPingPong()
{
	// public API kept for external use if needed:
	dxManager_->GetCommandContext()->TransitionResource(
		pingPongBuffers_[ping_].Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// set RTV & DSV
	dxManager_->GetCommandContext()->SetRenderTarget(rtvHandles_[ping_], dxManager_->GetDsvManager()->GetDsvHandle());
	dxManager_->GetCommandContext()->ClearRenderTarget(rtvHandles_[ping_], clearValue_.Color);
	dxManager_->GetCommandContext()->SetViewportAndScissor(viewport_, scissorRect_);
}

void OffScreenManager::EndDrawToPingPong()
{
	dxManager_->GetCommandContext()->TransitionResource(
		pingPongBuffers_[ping_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);

	std::swap(ping_, pong_);
}

Microsoft::WRL::ComPtr<ID3D12Resource> OffScreenManager::CreateOffScreenRenderTarget()
{
	return dxManager_->CreateRenderTextureResource(WindowManager::kClientWidth, WindowManager::kClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
}

uint32_t OffScreenManager::CreateRTVForResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
	// allocate index from RtvManager
	uint32_t index = dxManager_->GetRtvManager()->Allocate();
	// create RTV at manager
	dxManager_->GetRtvManager()->CreateRTV(index, resource.Get());
	return index;
}

uint32_t OffScreenManager::CreateSRVForResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
	uint32_t index = dxManager_->GetSrvManager()->Allocate();
	dxManager_->GetSrvManager()->CreateSRVforTexture2D(index, resource.Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1);
	return index;
}
