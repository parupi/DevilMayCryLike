#pragma once

#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#include "Graphics/Rendering/Sprite/Sprite.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Resource/TextureManager.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Object/Model/Model.h"
#include "World3D/Object/Model/ModelLoader.h"
#include "World3D/Object/Model/ModelManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Core/GuchisFramework.h"
#include "Debugger/ImGuiManager.h"
#include "Debugger/GlobalVariables.h"

class MyGameTitle : public GuchisFramework
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
	// オブジェクト削除
	void RemoveObjects() override;
};

