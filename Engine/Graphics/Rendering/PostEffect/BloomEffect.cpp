#include "BloomEffect.h"
#include "OffScreenManager.h"
#include "Graphics/Resource/SrvManager.h"
#include "Platform/WindowManager.h"

namespace {
	// 作業用RTの縮小率。半解像度にすると1回のブラーで倍の広がりが得られる
	constexpr uint32_t kDownScale = 2;
	// 作業用RTのフォーマット（チェーンのping-pongに合わせる）
	constexpr DXGI_FORMAT kWorkFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
}

BloomEffect::BloomEffect(const std::string& name) {
	name_ = name;
	dxManager_ = OffScreenManager::GetInstance().GetDXManager();
	psoManager_ = OffScreenManager::GetInstance().GetPSOManager();

	CreateWorkTargets();
	CreateEffectResource();
}

BloomEffect::~BloomEffect() {
	bright_.resource.Reset();
	blur_.resource.Reset();

	dxManager_ = nullptr;
	psoManager_ = nullptr;
}

void BloomEffect::CreateWorkTargets() {
	workWidth_ = WindowManager::kGameWidth / kDownScale;
	workHeight_ = WindowManager::kGameHeight / kDownScale;

	workViewport_ = {0.0f, 0.0f, static_cast<float>(workWidth_), static_cast<float>(workHeight_), 0.0f, 1.0f};
	workScissor_ = {0, 0, static_cast<LONG>(workWidth_), static_cast<LONG>(workHeight_)};

	auto* rtvManager = dxManager_->GetRtvManager();
	auto* srvManager = dxManager_->GetSrvManager();

	WorkTarget* targets[] = {&bright_, &blur_};
	for (WorkTarget* target : targets) {
		GpuResourceFactory::TextureDesc desc;
		desc.width = workWidth_;
		desc.height = workHeight_;
		desc.format = kWorkFormat;
		desc.usage = GpuResourceFactory::Usage::RenderTarget;
		desc.clearColor[0] = 0.0f;
		desc.clearColor[1] = 0.0f;
		desc.clearColor[2] = 0.0f;
		desc.clearColor[3] = 1.0f;

		target->resource = dxManager_->GetResourceFactory()->CreateTexture2D(desc);

		const uint32_t rtvIndex = rtvManager->Allocate();
		rtvManager->CreateRTV(rtvIndex, target->resource.Get());
		target->rtv = rtvManager->GetCPUDescriptorHandle(rtvIndex);

		const uint32_t srvIndex = srvManager->Allocate();
		srvManager->CreateSRVforTexture2D(srvIndex, target->resource.Get(), kWorkFormat, 1);
		target->srv = srvManager->GetGPUDescriptorHandle(srvIndex);

		// 生成直後は RENDER_TARGET なので、他のRTと同じく読み取り状態に揃えておく
		dxManager_->GetCommandContext()->TransitionResource(
			target->resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
	}
}

void BloomEffect::CreateEffectResource() {
	auto* resourceManager = dxManager_->GetResourceManager();

	brightParamHandle_ = resourceManager->CreateUploadBuffer(sizeof(BrightParam), L"BloomBrightParam");
	brightParam_ = reinterpret_cast<BrightParam*>(resourceManager->Map(brightParamHandle_));

	// 横と縦で違う値を同じフレームに使うため、ブラー用のバッファは2つ分ける
	blurHParamHandle_ = resourceManager->CreateUploadBuffer(sizeof(BlurParam), L"BloomBlurParamH");
	blurHParam_ = reinterpret_cast<BlurParam*>(resourceManager->Map(blurHParamHandle_));

	blurVParamHandle_ = resourceManager->CreateUploadBuffer(sizeof(BlurParam), L"BloomBlurParamV");
	blurVParam_ = reinterpret_cast<BlurParam*>(resourceManager->Map(blurVParamHandle_));

	compositeParamHandle_ = resourceManager->CreateUploadBuffer(sizeof(CompositeParam), L"BloomCompositeParam");
	compositeParam_ = reinterpret_cast<CompositeParam*>(resourceManager->Map(compositeParamHandle_));

	// 初期値を定数バッファへ流し込む
	ApplyParams();
}

void BloomEffect::Update() {
	ApplyParams();
}

void BloomEffect::ApplyParams() {
	brightParam_->threshold = settings_.threshold;
	brightParam_->softKnee = settings_.softKnee;

	// 作業用RTのテクセルサイズを基準にオフセットを決める
	const float texelU = settings_.blurRadius / static_cast<float>(workWidth_);
	const float texelV = settings_.blurRadius / static_cast<float>(workHeight_);
	blurHParam_->direction = {texelU, 0.0f};
	blurVParam_->direction = {0.0f, texelV};

	compositeParam_->intensity = settings_.intensity;
}

void BloomEffect::DrawPass(OffScreenEffectType type, D3D12_GPU_DESCRIPTOR_HANDLE input,
	const WorkTarget& output, uint32_t cbvHandle) {
	auto* commandList = dxManager_->GetCommandList();
	auto* context = dxManager_->GetCommandContext();

	context->TransitionResource(output.resource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	context->SetRenderTarget(output.rtv);
	context->SetViewportAndScissor(workViewport_, workScissor_);

	commandList->SetPipelineState(psoManager_->GetOffScreenPSO(type));
	commandList->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());
	commandList->SetGraphicsRootDescriptorTable(0, input);
	commandList->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(cbvHandle));
	commandList->DrawInstanced(3, 1, 0, 0);

	context->TransitionResource(output.resource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void BloomEffect::Draw() {
	auto* commandList = dxManager_->GetCommandList();
	auto* context = dxManager_->GetCommandContext();

	// 1. 明るい部分を抽出（入力 → bright、半解像度）
	DrawPass(OffScreenEffectType::kBloomBright, inputSrv_, bright_, brightParamHandle_);

	// 2. 横ブラー（bright → blur）
	DrawPass(OffScreenEffectType::kBloomBlur, bright_.srv, blur_, blurHParamHandle_);

	// 3. 縦ブラー（blur → bright）
	DrawPass(OffScreenEffectType::kBloomBlur, blur_.srv, bright_, blurVParamHandle_);

	// 4. チェーンの出力先へ戻す（フル解像度）
	const OffScreenManager& offScreen = OffScreenManager::GetInstance();
	context->SetRenderTarget(outputRtv_);
	context->SetViewportAndScissor(offScreen.GetViewport(), offScreen.GetScissorRect());

	commandList->SetGraphicsRootSignature(psoManager_->GetOffScreenSignature());

	// 4a. まず元の絵をそのままコピーする
	commandList->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kNone));
	commandList->SetGraphicsRootDescriptorTable(0, inputSrv_);
	commandList->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(compositeParamHandle_));
	commandList->DrawInstanced(3, 1, 0, 0);

	// 4b. ぼかした光を加算ブレンドで重ねる
	commandList->SetPipelineState(psoManager_->GetOffScreenPSO(OffScreenEffectType::kBloomComposite));
	commandList->SetGraphicsRootDescriptorTable(0, bright_.srv);
	commandList->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(compositeParamHandle_));
	commandList->DrawInstanced(3, 1, 0, 0);
}
