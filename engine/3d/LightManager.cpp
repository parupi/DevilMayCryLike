#include "LightManager.h"
#include <Object3dManager.h>
#include <numbers>

void LightManager::Initialize()
{
	dxManager_ = Object3dManager::GetInstance()->GetDxManager();

	CreateDirLightResource();
	CreatePointLightResource();
	CreateSpotLightResource();
}

void LightManager::BindLightsToShader()
{
	auto commandList = dxManager_->GetCommandList();

	// ディレクションライトのリソースをシェーダーにバインド
	commandList->SetGraphicsRootConstantBufferView(4, dirLightResource1_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(5, dirLightResource2_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(6, dirLightResource3_->GetGPUVirtualAddress());
	// ポイントライトのリソースをシェーダーにバインド
	commandList->SetGraphicsRootConstantBufferView(7, pointLightResource1_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(8, pointLightResource2_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(9, pointLightResource3_->GetGPUVirtualAddress());
	// スポットライトのリソースをシェーダーにバインド
	commandList->SetGraphicsRootConstantBufferView(10, spotLightResource1_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(11, spotLightResource2_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(12, spotLightResource3_->GetGPUVirtualAddress());
}

void LightManager::CreateDirLightResource()
{
	dirLightResource1_ = dxManager_->CreateBufferResource(sizeof(DirectionalLight));
	dirLightResource1_->Map(0, nullptr, reinterpret_cast<void**>(&dirLightData1_));
	dirLightResource2_ = dxManager_->CreateBufferResource(sizeof(DirectionalLight));
	dirLightResource2_->Map(0, nullptr, reinterpret_cast<void**>(&dirLightData2_));
	dirLightResource3_ = dxManager_->CreateBufferResource(sizeof(DirectionalLight));
	dirLightResource3_->Map(0, nullptr, reinterpret_cast<void**>(&dirLightData3_));
	dirLightData1_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	dirLightData1_->direction = { 0.0f, -2.0f, 1.0f };
	dirLightData1_->enabled = false;
	dirLightData1_->intensity = 1.0f;
	dirLightData2_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	dirLightData2_->direction = { 0.0f, -2.0f, 1.0f };
	dirLightData2_->enabled = false;
	dirLightData2_->intensity = 1.0f;
	dirLightData3_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	dirLightData3_->direction = { 0.0f, -2.0f, 1.0f };
	dirLightData3_->enabled = false;
	dirLightData3_->intensity = 1.0f;
}

void LightManager::CreatePointLightResource()
{
	pointLightResource1_ = dxManager_->CreateBufferResource(sizeof(PointLight));
	pointLightResource1_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData1_));
	pointLightResource2_ = dxManager_->CreateBufferResource(sizeof(PointLight));
	pointLightResource2_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData2_));
	pointLightResource3_ = dxManager_->CreateBufferResource(sizeof(PointLight));
	pointLightResource3_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData3_));
	pointLightData1_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pointLightData1_->decay = 1.0f;
	pointLightData1_->enabled = false;
	pointLightData1_->intensity = 1.0f;
	pointLightData1_->position = { 0.0f, 2.0f, 0.0f };
	pointLightData1_->radius = 10.0f;
	pointLightData2_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pointLightData2_->decay = 1.0f;
	pointLightData2_->enabled = false;
	pointLightData2_->intensity = 1.0f;
	pointLightData2_->position = { 0.0f, 2.0f, 0.0f };
	pointLightData2_->radius = 10.0f;
	pointLightData3_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pointLightData3_->decay = 1.0f;
	pointLightData3_->enabled = false;
	pointLightData3_->intensity = 1.0f;
	pointLightData3_->position = { 0.0f, 2.0f, 0.0f };
	pointLightData3_->radius = 10.0f;
}

void LightManager::CreateSpotLightResource()
{
	spotLightResource1_ = dxManager_->CreateBufferResource(sizeof(SpotLight));
	spotLightResource1_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData1_));
	spotLightResource2_ = dxManager_->CreateBufferResource(sizeof(SpotLight));
	spotLightResource2_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData2_));
	spotLightResource3_ = dxManager_->CreateBufferResource(sizeof(SpotLight));
	spotLightResource3_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData3_));
	spotLightData1_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	spotLightData1_->position = { 2.0f, 1.25f, 0.0f };
	spotLightData1_->distance = 7.0f;
	spotLightData1_->direction = Normalize(Vector3(- 1.0f, -1.0f, 0.0f));
	spotLightData1_->intensity = 4.0f;
	spotLightData1_->decay = 2.0f;
	spotLightData1_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
	spotLightData2_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	spotLightData2_->position = { 2.0f, 1.25f, 0.0f };
	spotLightData2_->distance = 7.0f;
	spotLightData2_->direction = Normalize(Vector3(-1.0f, -1.0f, 0.0f));
	spotLightData2_->intensity = 4.0f;
	spotLightData2_->decay = 2.0f;
	spotLightData2_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
	spotLightData3_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	spotLightData3_->position = { 2.0f, 1.25f, 0.0f };
	spotLightData3_->distance = 7.0f;
	spotLightData3_->direction = Normalize(Vector3(-1.0f, -1.0f, 0.0f));
	spotLightData3_->intensity = 4.0f;
	spotLightData3_->decay = 2.0f;
	spotLightData3_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
}

void LightManager::SetDirLightColor(uint32_t index, const Vector4& color)
{
	if (index == 0) {
		dirLightData1_->color = color;
	}
	else if (index == 1) {
		dirLightData2_->color = color;
	}
	else {
		dirLightData3_->color = color;
	}
}

Vector3 LightManager::GetDirLightDirection(uint32_t index) const
{
	if (index == 0) {
		return dirLightData1_->direction;
	}
	else if (index == 1) {
		return dirLightData2_->direction;
	}
	else {
		return dirLightData3_->direction;
	}
}

Vector4 LightManager::GetDirLightColor(uint32_t index) const
{
	if (index == 0) {
		return dirLightData1_->color;
	}
	else if (index == 1) {
		return dirLightData2_->color;
	}
	else {
		return dirLightData3_->color;
	}
}

void LightManager::SetDirLightIntensity(uint32_t index, float intensity)
{
	if (index == 0) {
		dirLightData1_->intensity = intensity;
	}
	else if (index == 1) {
		dirLightData2_->intensity = intensity;
	}
	else {
		dirLightData3_->intensity = intensity;
	}
}

bool LightManager::GetDirLightActive(uint32_t index) const
{
	if (index == 0) {
		return dirLightData1_->enabled;
	}
	else if (index == 1) {
		return dirLightData2_->enabled;
	}
	else {
		return dirLightData3_->enabled;
	}
}

void LightManager::SetDirLightActive(uint32_t index, bool isActive)
{
	if (index == 0) {
		dirLightData1_->enabled = isActive;
	}
	else if (index == 1) {
		dirLightData2_->enabled = isActive;
	}
	else {
		dirLightData3_->enabled = isActive;
	}
}

void LightManager::SetDirLightDirection(uint32_t index, const Vector3& dir)
{
	if (index == 0) {
		dirLightData1_->direction = dir;
	}
	else if (index == 1) {
		dirLightData2_->direction = dir;
	}
	else {
		dirLightData3_->direction = dir;
	}
}

float LightManager::GetDirLightIntensity(uint32_t index) const
{
	if (index == 0) {
		return dirLightData1_->intensity;
	}
	else if (index == 1) {
		return dirLightData2_->intensity;
	}
	else {
		return dirLightData3_->intensity;
	}
}

void LightManager::SetPointLightActive(uint32_t index, bool isActive)
{
	if (index == 0) {
		pointLightData1_->enabled = isActive;
	}
	else if (index == 1) {
		pointLightData2_->enabled = isActive;
	}
	else {
		pointLightData3_->enabled = isActive;
	}
}

Vector4 LightManager::GetPointLightColor(uint32_t index) const
{
	if (index == 0) {
		return pointLightData1_->color;
	}
	else if (index == 1) {
		return pointLightData2_->color;
	}
	else {
		return pointLightData3_->color;
	}
}

void LightManager::SetPointLightColor(uint32_t index, const Vector4& color)
{
	if (index == 0) {
		pointLightData1_->color = color;
	}
	else if (index == 1) {
		pointLightData2_->color = color;
	}
	else {
		pointLightData3_->color = color;
	}
}

Vector3 LightManager::GetPointLightPos(uint32_t index) const
{
	if (index == 0) {
		return pointLightData1_->position;
	}
	else if (index == 1) {
		return pointLightData2_->position;
	}
	else {
		return pointLightData3_->position;
	}
}

void LightManager::SetPointLightPos(uint32_t index, const Vector3& pos)
{
	if (index == 0) {
		pointLightData1_->position = pos;
	}
	else if (index == 1) {
		pointLightData2_->position = pos;
	}
	else {
		pointLightData3_->position = pos;
	}
}

float LightManager::GetPointLightIntensity(uint32_t index) const
{
	if (index == 0) {
		return pointLightData1_->intensity;
	}
	else if (index == 1) {
		return pointLightData2_->intensity;
	}
	else {
		return pointLightData3_->intensity;
	}
}

void LightManager::SetPointLightIntensity(uint32_t index, float intensity)
{
	if (index == 0) {
		pointLightData1_->intensity = intensity;
	}
	else if (index == 1) {
		pointLightData2_->intensity = intensity;
	}
	else {
		pointLightData3_->intensity = intensity;
	}
}

float LightManager::GetPointLightDecay(uint32_t index) const
{
	if (index == 0) {
		return pointLightData1_->decay;
	}
	else if (index == 1) {
		return pointLightData2_->decay;
	}
	else {
		return pointLightData3_->decay;
	}
}

float LightManager::GetPointLightRadius(uint32_t index) const
{
	if (index == 0) {
		return pointLightData1_->radius;
	}
	else if (index == 1) {
		return pointLightData2_->radius;
	}
	else {
		return pointLightData3_->radius;
	}
}

void LightManager::SetPointLightRadius(uint32_t index, float radius)
{
	if (index == 0) {
		pointLightData1_->radius = radius;
	}
	else if (index == 1) {
		pointLightData2_->radius = radius;
	}
	else {
		pointLightData3_->radius = radius;
	}
}

void LightManager::SetPointLightDecay(uint32_t index, float decay)
{
	if (index == 0) {
		pointLightData1_->decay = decay;
	}
	else if (index == 1) {
		pointLightData2_->decay = decay;
	}
	else {
		pointLightData3_->decay = decay;
	}
}

bool LightManager::GetPointLightActive(uint32_t index) const
{
	if (index == 0) {
		return pointLightData1_->enabled;
	}
	else if (index == 1) {
		return pointLightData2_->enabled;
	}
	else {
		return pointLightData3_->enabled;
	}
}

void LightManager::SetSpotLightColor(uint32_t index, const Vector4& color)
{
	if (index == 0) {
		spotLightData1_->color = color;
	}
	else if (index == 1) {
		spotLightData2_->color = color;
	}
	else {
		spotLightData3_->color = color;
	}
}

void LightManager::SetSpotLightPos(uint32_t index, const Vector3& pos)
{
	if (index == 0) {
		spotLightData1_->position = pos;
	}
	else if (index == 1) {
		spotLightData2_->position = pos;
	}
	else {
		spotLightData3_->position = pos;
	}
}

void LightManager::SetSpotLightIntensity(uint32_t index, float intensity)
{
	if (index == 0) {
		spotLightData1_->intensity = intensity;
	}
	else if (index == 1) {
		spotLightData2_->intensity = intensity;
	}
	else {
		spotLightData3_->intensity = intensity;
	}
}

void LightManager::SetSpotLightDirection(uint32_t index, const Vector3& dir)
{
	if (index == 0) {
		spotLightData1_->direction = dir;
	}
	else if (index == 1) {
		spotLightData2_->direction = dir;
	}
	else {
		spotLightData3_->direction = dir;
	}
}

void LightManager::SetSpotLightDistance(uint32_t index, float distance)
{
	if (index == 0) {
		spotLightData1_->distance = distance;
	}
	else if (index == 1) {
		spotLightData2_->distance = distance;
	}
	else {
		spotLightData3_->distance = distance;
	}
}

void LightManager::SetSpotLightDecay(uint32_t index, float decay)
{
	if (index == 0) {
		spotLightData1_->decay = decay;
	}
	else if (index == 1) {
		spotLightData2_->decay = decay;
	}
	else {
		spotLightData3_->decay = decay;
	}
}

void LightManager::SetSpotLightCosAngle(uint32_t index, float cosAngle)
{
	if (index == 0) {
		spotLightData1_->cosAngle = cosAngle;
	}
	else if (index == 1) {
		spotLightData2_->cosAngle = cosAngle;
	}
	else {
		spotLightData3_->cosAngle = cosAngle;
	}
}

void LightManager::SetSpotLightActive(uint32_t index, bool isActive)
{
	if (index == 0) {
		spotLightData1_->enabled = isActive;
	}
	else if (index == 1) {
		spotLightData2_->enabled = isActive;
	}
	else {
		spotLightData3_->enabled = isActive;
	}
}

// SpotLightのゲッターの定義
Vector4 LightManager::GetSpotLightColor(uint32_t index) const
{
	if (index == 0) {
		return spotLightData1_->color;
	}
	else if (index == 1) {
		return spotLightData2_->color;
	}
	else {
		return spotLightData3_->color;
	}
}

Vector3 LightManager::GetSpotLightPos(uint32_t index) const
{
	if (index == 0) {
		return spotLightData1_->position;
	}
	else if (index == 1) {
		return spotLightData2_->position;
	}
	else {
		return spotLightData3_->position;
	}
}

float LightManager::GetSpotLightIntensity(uint32_t index) const
{
	if (index == 0) {
		return spotLightData1_->intensity;
	}
	else if (index == 1) {
		return spotLightData2_->intensity;
	}
	else {
		return spotLightData3_->intensity;
	}
}

Vector3 LightManager::GetSpotLightDirection(uint32_t index) const
{
	if (index == 0) {
		return spotLightData1_->direction;
	}
	else if (index == 1) {
		return spotLightData2_->direction;
	}
	else {
		return spotLightData3_->direction;
	}
}

float LightManager::GetSpotLightDistance(uint32_t index) const
{
	if (index == 0) {
		return spotLightData1_->distance;
	}
	else if (index == 1) {
		return spotLightData2_->distance;
	}
	else {
		return spotLightData3_->distance;
	}
}

float LightManager::GetSpotLightDecay(uint32_t index) const
{
	if (index == 0) {
		return spotLightData1_->decay;
	}
	else if (index == 1) {
		return spotLightData2_->decay;
	}
	else {
		return spotLightData3_->decay;
	}
}

float LightManager::GetSpotLightCosAngle(uint32_t index) const
{
	if (index == 0) {
		return spotLightData1_->cosAngle;
	}
	else if (index == 1) {
		return spotLightData2_->cosAngle;
	}
	else {
		return spotLightData3_->cosAngle;
	}
}

bool LightManager::GetSpotLightActive(uint32_t index) const
{
	if (index == 0) {
		return spotLightData1_->enabled;
	}
	else if (index == 1) {
		return spotLightData2_->enabled;
	}
	else {
		return spotLightData3_->enabled;
	}
}