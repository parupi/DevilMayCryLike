#pragma once

#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#include "Sprite.h"
#include "SpriteManager.h"
#include "TextureManager.h"
#include "Object3d.h"
#include "Object3dmanager.h"
#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"
#include <ParticleManager.h>
#include "GuchisFramework.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"

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
};

