#pragma once
#include <World3D/Object/Object3d.h>
#include <Graphics/Rendering/Sprite/Sprite.h>
#include <World3D/Object/Model/Model.h>
#include <vector>
#include "World3D/Camera/BaseCamera.h"
#include <Scene/BaseScene.h>
#include <memory>
#include <Audio/Audio.h>
#include <World3D/Camera/CameraManager.h>
#include <World3D/WorldTransform.h>
#include <Graphics/Rendering/Particle/ParticleEmitter.h>
#include "Graphics/Rendering/PostEffect/GrayEffect.h"
#include <World3D/Light/LightManager.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include <World3D/Object/Renderer/PrimitiveRenderer.h>
#include <World3D/Collider/AABBCollider.h>
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Camera/GameCamera.h"
#include <imgui_node_editor.h>

using NodeID = uint32_t;
using PinID = uint32_t;
using LinkID = uint32_t;

struct ImGuiNode
{
	NodeID id;
	PinID  inputPin;
	PinID  outputPin;
	const char* name;
};

struct Link
{
	LinkID id;
	PinID  from;
	PinID  to;
};

class SampleScene : public BaseScene
{
public:
	// 初期化
	void Initialize() override;
	// 終了
	void Finalize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;

#ifdef _DEBUG
	void DebugUpdate() override;
#endif // _DEBUG

private:
	CameraManager* cameraManager_ = &CameraManager::GetInstance();

	std::unique_ptr<BaseCamera> normalCamera_;

	LightManager* lightManager_ = &LightManager::GetInstance();

	Object3d* object_ = nullptr;
	Object3d* object2_ = nullptr;

	Sprite* sprite_ = nullptr;

	DirectionalLight* dirLight_ = nullptr;

	ParticleEmitter* emitter_ = nullptr;
};

