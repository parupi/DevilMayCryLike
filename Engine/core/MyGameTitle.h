#pragma once

#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#include "2d/Sprite.h"
#include "2d/SpriteManager.h"
#include "base/TextureManager.h"
#include "3d/Object/Object3d.h"
#include "3d/Object/Object3dManager.h"
#include "3d/Object/Model/Model.h"
#include "3d/Object/Model/ModelLoader.h"
#include "3d/Object/Model/ModelManager.h"
#include "base/Particle/ParticleManager.h"
#include "core/GuchisFramework.h"
#include "debuger/ImGuiManager.h"
#include "debuger/GlobalVariables.h"

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

