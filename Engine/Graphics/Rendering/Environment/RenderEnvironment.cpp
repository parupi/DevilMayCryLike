#include "RenderEnvironment.h"
#include "Graphics/Device/DirectXManager.h"
#include <cmath>

namespace {
// sRGB の値（0〜1）をリニアへ
float SrgbToLinear(float c)
{
	return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
} // namespace

RenderEnvironment& RenderEnvironment::GetInstance()
{
	static RenderEnvironment instance;
	return instance;
}

void RenderEnvironment::Initialize(DirectXManager* dxManager)
{
	// 定数バッファは256バイト境界
	buffer_ = dxManager->GetResourceManager()->CreateUploadResource((sizeof(EnvironmentCB) + 255) & ~255);
	buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped_));
	*mapped_ = EnvironmentCB{};
}

void RenderEnvironment::Finalize()
{
	if (buffer_ && mapped_) {
		buffer_->Unmap(0, nullptr);
	}
	mapped_ = nullptr;
	buffer_.Reset();
}

Vector3 RenderEnvironment::GetBackgroundLinear() const
{
	return {
		SrgbToLinear(params_.backgroundColor.x),
		SrgbToLinear(params_.backgroundColor.y),
		SrgbToLinear(params_.backgroundColor.z),
	};
}

void RenderEnvironment::BindToLighting(ID3D12GraphicsCommandList* cmd, UINT rootParameterIndex)
{
	if (!mapped_) return;

	const Vector3 bg = GetBackgroundLinear();
	EnvironmentCB cb{};
	cb.backgroundColor[0] = bg.x;
	cb.backgroundColor[1] = bg.y;
	cb.backgroundColor[2] = bg.z;
	cb.ambientColor[0] = params_.ambientColor.x;
	cb.ambientColor[1] = params_.ambientColor.y;
	cb.ambientColor[2] = params_.ambientColor.z;
	cb.ambientAlbedoMix = params_.ambientAlbedoMix;
	cb.directionalScale[0] = params_.directionalScale.x;
	cb.directionalScale[1] = params_.directionalScale.y;
	cb.directionalScale[2] = params_.directionalScale.z;
	cb.darkStartHeight = params_.darkStartHeight;
	cb.darkEndHeight = params_.darkEndHeight;
	cb.darkStrength = params_.heightDarkEnabled ? params_.darkStrength : 0.0f;
	*mapped_ = cb;

	cmd->SetGraphicsRootConstantBufferView(rootParameterIndex, buffer_->GetGPUVirtualAddress());
}
