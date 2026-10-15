#include "RenderEnvironment.h"
#include "Graphics/Device/DirectXManager.h"
#include "World3D/Camera/CameraManager.h"
#include "World3D/Camera/BaseCamera.h"
#include "Math/MathUtils.h"
#include "Utility/TimeManager.h"
#include <cstring>
#include <algorithm>
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

Vector3 RenderEnvironment::ToLinear(const Vector3& srgb)
{
	return { SrgbToLinear(srgb.x), SrgbToLinear(srgb.y), SrgbToLinear(srgb.z) };
}

Vector3 RenderEnvironment::GetBackgroundLinear() const
{
	return ToLinear(params_.backgroundColor);
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
	const Vector3 fogColor = ToLinear(params_.heightFogColor);
	cb.heightFogColor[0] = fogColor.x;
	cb.heightFogColor[1] = fogColor.y;
	cb.heightFogColor[2] = fogColor.z;
	cb.heightFogDensity = params_.heightFogEnabled ? params_.heightFogDensity : 0.0f;
	cb.heightFogBaseHeight = params_.heightFogBaseHeight;
	cb.heightFogFalloff = params_.heightFogFalloff;
	cb.heightFogMaxOpacity = params_.heightFogMaxOpacity;
	const Vector3 distanceFogColor = ToLinear(params_.distanceFogColor);
	cb.distanceFogColor[0] = distanceFogColor.x;
	cb.distanceFogColor[1] = distanceFogColor.y;
	cb.distanceFogColor[2] = distanceFogColor.z;
	cb.distanceFogDensity = params_.distanceFogEnabled ? params_.distanceFogDensity : 0.0f;
	cb.distanceFogStart = params_.distanceFogStart;
	cb.distanceFogMaxOpacity = params_.distanceFogMaxOpacity;
	cb.characterAmbient[0] = params_.characterAmbient.x;
	cb.characterAmbient[1] = params_.characterAmbient.y;
	cb.characterAmbient[2] = params_.characterAmbient.z;
	cb.rimPower = params_.rimPower;
	cb.rimColor[0] = params_.rimColor.x;
	cb.rimColor[1] = params_.rimColor.y;
	cb.rimColor[2] = params_.rimColor.z;
	const bool ssao = params_.ssaoEnabled && aoSrv_.ptr != 0;
	cb.ssaoAmbientStrength = ssao ? params_.ssaoAmbientStrength : 0.0f;
	cb.ssaoDirectStrength = ssao ? params_.ssaoDirectStrength : 0.0f;
	cb.volumetricStrength = params_.volumetricEnabled ? params_.volumetricStrength : 0.0f;
	cb.volumetricMaxDistance = params_.volumetricMaxDistance;
	cb.spotVolumetricStrength = params_.volumetricEnabled ? params_.spotVolumetricStrength : 0.0f;
	cb.spotVolumetricSteps = static_cast<float>((std::max)(params_.spotVolumetricSteps, 1));
	cb.spotFogNoiseScale = params_.spotFogNoiseScale;
	cb.spotFogNoiseInfluence = params_.spotFogNoiseInfluence;
	cb.spotFogNoiseSpeed[0] = params_.spotFogNoiseSpeed.x;
	cb.spotFogNoiseSpeed[1] = params_.spotFogNoiseSpeed.y;
	cb.spotFogNoiseSpeed[2] = params_.spotFogNoiseSpeed.z;
	cb.spotPhaseG = params_.spotPhaseG;
	cb.spotApexFade = params_.spotApexFade;
	cb.debugView = static_cast<float>(static_cast<int>(debugView_));
	// 霧の濃淡はヒットストップ中は止まるよう VFX の時間で進める
	time_ += TimeManager::GetVFXDelta();
	cb.time = time_;
	if (BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera()) {
		const Matrix4x4 inv = Inverse(camera->GetViewProjectionMatrix());
		std::memcpy(cb.invViewProj, &inv, sizeof(cb.invViewProj));
	}
	*mapped_ = cb;

	cmd->SetGraphicsRootConstantBufferView(rootParameterIndex, buffer_->GetGPUVirtualAddress());
	if (aoSrv_.ptr != 0) {
		cmd->SetGraphicsRootDescriptorTable(rootParameterIndex + 1, aoSrv_);
	}
}
